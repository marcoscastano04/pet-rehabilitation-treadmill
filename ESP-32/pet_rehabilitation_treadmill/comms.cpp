#include "comms.h"
#include "globals.h"
#include "driver/uart.h"   // Para usar uart_write_bytes
#include <string.h>

// Delimitadores y carácter de escape del protocolo de comunicación.
#define START_BYTE  0xAA
#define END_BYTE    0x55
#define ESC_BYTE    0x7E
// Códigos de escape para representar bytes reservados dentro del paquete.
#define ESC_AA      0x01
#define ESC_55      0x02
#define ESC_ESC     0x7E

// Estados de la máquina de estados de recepción.
enum RxState {
  WAIT_START,
  READ_DATA,
  ESCAPE
};

static RxState rxState = WAIT_START;

// Búfer de recepción y posición de escritura.
static uint8_t rxBuffer[MAX_RX_BUFFER];
static uint8_t rxIndex = 0;

// Estado y búfer de transmisión.
static volatile bool txActive = false;
static volatile uint8_t txIndex = 0;
static volatile uint8_t txLen = 0;
static char txBuffer[MAX_TX_BUFFER];

static hw_timer_t *txTimer = NULL;
static portMUX_TYPE txMux = portMUX_INITIALIZER_UNLOCKED;

// Declaración de la rutina de interrupción del temporizador de transmisión.
void IRAM_ATTR onTxTimer();
// Transmitir un byte por interrupción hasta completar el comando.
void IRAM_ATTR onTxTimer() {
  portENTER_CRITICAL_ISR(&txMux);

  if (!txActive) {
    portEXIT_CRITICAL_ISR(&txMux);
    return;
  }

  uart_write_bytes(UART_NUM_2, &txBuffer[txIndex], 1);
  txIndex++;

  if (txIndex >= txLen) {
    txActive = false;
    timerStop(txTimer);   //termina tx
  }

  portEXIT_CRITICAL_ISR(&txMux);
}

void initCommsTimers() {
  if (txTimer == NULL) {
    txTimer = timerBegin(1000000); // 1 MHz
    timerAttachInterrupt(txTimer, &onTxTimer);
    timerAlarm(txTimer, 90, true, 0); // Intervalo entre bytes de 90 µs periodicos.
    timerStop(txTimer);
  }
}

void sendCommandSpaced(const char *cmd) {
  portENTER_CRITICAL(&txMux);

  if (txActive) {
    portEXIT_CRITICAL(&txMux);
    return;     // Evitar sobrescribir una transmisión en curso.
  }

  txLen = strlen(cmd);
  if (txLen > sizeof(txBuffer)) txLen = sizeof(txBuffer);

  // Cargar el comando en el búfer e iniciar la transmisión temporizada.
  memcpy(txBuffer, cmd, txLen);
  txIndex = 0;
  txActive = true;

  if (txTimer == NULL) initCommsTimers();
  timerWrite(txTimer, 0);
  timerStart(txTimer);

  portEXIT_CRITICAL(&txMux);
}

// Procesamiento de paquetes (espera paquetes sin framing)
void processPacket(uint8_t *d, uint8_t len) {

  // Payload esperado:
  if (len != MAX_RX_PAYLOAD) {
    Serial.println("Paquete invalido (len)");
    return;
  }

  // Guardar previo
  td_prev = td;
  uint8_t idx = 0;

  // Decodificar los campos del paquete y actualizar la estructura de telemetría.
  // memcpy(destino,origen,cantidad) copia byte a byte
  memcpy(&td.speed_measured, &d[idx], 4); idx += 4;
  td.ic3_buffer = d[idx] | (d[idx + 1] << 8); idx += 2;
  td.speed_setpoint_u = (int16_t)(d[idx] | (d[idx + 1] << 8)); idx += 2;
  td.speed_setpoint_m = (int16_t)(d[idx] | (d[idx + 1] << 8)); idx += 2;
  memcpy(&td.salida_y, &d[idx], 4); idx += 4;
  memcpy(&td.error, &d[idx], 4); idx += 4;
  memcpy(&td.current, &d[idx], 4); idx += 4;
  memcpy(&td.acumulador_integral, &d[idx], 4); idx += 4;
  memcpy(&td.bus_voltage, &d[idx], 4); idx += 4;
  memcpy(&td.temperature, &d[idx], 4); idx += 4;
  memcpy(&td.t1_ticks, &d[idx], 4);

  // Actualizar la pantalla si cambió la telemetría mostrada.
  checkTelemetryChanges();

  // Imprimir cada captura
  if (ButtonText == false){
    Serial.printf("%u,%u,%.2f,%u,%d,%d,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
    minutes,
    seconds,
    td.speed_measured,
    td.ic3_buffer,
    td.speed_setpoint_u,
    td.speed_setpoint_m,
    td.salida_y,
    td.error,
    td.current,
    td.acumulador_integral,
    td.bus_voltage,
    td.temperature,
    td.t1_ticks);
  }
}

// Reconstruir los paquetes recibidos mediante una máquina de estados.
void processByte(uint8_t b) {
  switch (rxState) {

    case WAIT_START:
      if (b == START_BYTE) {
        rxIndex = 0;
        rxState = READ_DATA;
      }
      break;

    case READ_DATA:
      // Si START inesperado, reset y nuevo paquete
      if (b == START_BYTE) {
        rxIndex = 0;
        break;
      }

      if (b == END_BYTE) {
        processPacket(rxBuffer, rxIndex);
        rxState = WAIT_START;
      }
      else if (b == ESC_BYTE) {
        rxState = ESCAPE;
      }
      else {
        if (rxIndex < MAX_RX_BUFFER) {
          rxBuffer[rxIndex++] = b;
        } else {
          // Overflow descarta paquete
          rxState = WAIT_START;
        }
      }
      break;

    case ESCAPE:
      if (rxIndex >= MAX_RX_BUFFER) {
        rxState = WAIT_START;
        break;
      }

      if (b == ESC_AA)      rxBuffer[rxIndex++] = START_BYTE;
      else if (b == ESC_55) rxBuffer[rxIndex++] = END_BYTE;
      else if (b == ESC_ESC)rxBuffer[rxIndex++] = ESC_BYTE;
      else {
        rxState = WAIT_START;
      }

      rxState = READ_DATA;
      break;
  }
}

void handleUsbCommand(const char *cmd) {

  // Ignorar comandos vacíos o nulos.
  if (cmd == NULL || cmd[0] == '\0') {
    return;
  }
  
  // Alternar el envío de telemetría por USB.
  if (strcmp(cmd, "write") == 0) {
    PIC_to_UART = !PIC_to_UART;
    Serial.println("Writing updated");
    return;
  }

  // Validar el formato del comando: destino seguido de un espacio y el mensaje.
  if (strlen(cmd) < 3 || cmd[1] != ' ') {
    Serial.println("Invalid format. Use 'P ' or 'B ' followed by message.");
    return;
  }

  char target = cmd[0];            // 'P' or 'B'
  const char *message = &cmd[2];

  // Identificar el destino y extraer el mensaje.
  if (target == 'P') {
    Serial.print("Forwarding to PIC: ");
    Serial.println(message);
    sendCommandSpaced(message);
  } else if (target == 'B') {
    Serial.print("Forwarding to Bluetooth: ");
    Serial.println(message);
    SerialBT.println(message);
  } else {
    Serial.println("Unknown target. Use P or B.");
  }
}