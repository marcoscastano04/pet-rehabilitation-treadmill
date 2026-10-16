#include "BluetoothSerial.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "driver/uart.h"
#include "globals.h"
#include "Display.h"
#include "buttons.h"
#include "bluetooth.h"
#include "comms.h"


// LED indicador de actividad
const int ledPin = 2;     
unsigned long previousMillis = 0;
const unsigned long interval = 1000;  // Intervalo de parpadeo (ms)

bool ledState = LOW;

// Configuración del dispositivo y parámetros de almacenamiento

const char *deviceName = "Caminadora";

#define MAX_VALUES 11
#define MAX_RECORDS 100  // Max number of stored entries
#define RXD2 16
#define TXD2 17

LiquidCrystal_I2C lcd(0x27, 16, 2);  // I2C LCD object   (single definition of lcd)



// Definiciones globales compartidas entre los módulos del firmware
TelemetryData td;
TelemetryData td_prev;
int currentScreen = 0;
const int maxScreens = 5;
bool screenNeedsRefresh = false;

bool PIC_to_UART = true;

// RX handling variables for USB
char usbBuffer[MAX_USB_PAYLOAD];
uint8_t usbIndex = 0;

// Timer/ISR para la temporización principal.
hw_timer_t *timer = NULL;
volatile SemaphoreHandle_t timerSemaphore;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

volatile uint32_t isrCounter = 0;
volatile uint32_t lastIsrAt = 0;

// Registra cada interrupción y notifica al programa principal.
void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  isrCounter = isrCounter + 1;
  lastIsrAt = millis();
  portEXIT_CRITICAL_ISR(&timerMux);
  xSemaphoreGiveFromISR(timerSemaphore, NULL);
}

//SETUP -----------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_UP_PIN, INPUT_PULLUP);
  pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);
  pinMode(BUTTON_FAST_PIN, INPUT_PULLUP);
  pinMode(BUTTON_SLOW_PIN, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);

  lcd.init();
  lcd.backlight();
  lcd.print("DeniroCorp");

  Serial2.begin(114943, SERIAL_8N1, RXD2, TXD2);  //UART2 para comunicación con PIC

  // Inicialización del temporizador periódico y su mecanismo de notificación
  timerSemaphore = xSemaphoreCreateBinary();
  timer = timerBegin(1000000);  //frecuencia 1Mhz
  timerAttachInterrupt(timer, &onTimer);
  // Interrupción cada 250 ms, de forma periódica (true), cantidad ilimitada (0).
  timerAlarm(timer, 250000, true, 0);
  
  // Inicio timers de comunicaciones (txTimer). 
  initCommsTimers();

  SerialBT.enableSSP(INPUT_CAPABILITY, OUTPUT_CAPABILITY);  
  SerialBT.onConfirmRequest(BTConfirmRequestCallback);
  SerialBT.onKeyRequest(BTKeyRequestCallback);
  SerialBT.onAuthComplete(BTAuthCompleteCallback);
  SerialBT.begin(deviceName);  

  Serial.printf("The device started with name \"%s\", now you can pair it with Bluetooth!\n", deviceName);
  if (INPUT_CAPABILITY and OUTPUT_CAPABILITY) {
    Serial.println("Both devices will display randomly generated code and if they match authenticate pairing on both devices");
  } else if (not INPUT_CAPABILITY and not OUTPUT_CAPABILITY) {
    Serial.println("Authenticate pairing on the other device. No PIN is used");
  } else if (not INPUT_CAPABILITY and OUTPUT_CAPABILITY) {
    Serial.println("Authenticate pairing on the other device. No PIN is used");
  } else if (INPUT_CAPABILITY and not OUTPUT_CAPABILITY) {
    Serial.println("After pairing is initiated you will be required to enter the passkey to the ESP32 device to authenticate\n > The Passkey will displayed on "
                   "the other device");
  }

  lcd.setCursor(0,1);
  displayScreen(currentScreen);
}


//LOOP  ------------------------------------------------------------------------------------

void loop() {

  // Cambiar el estado del LED
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;  // Guardar el tiempo actual
    ledState = !ledState;
    digitalWrite(ledPin, ledState);
  }
  
  // Recepción y procesamiento de datos enviados por la PIC
  while (Serial2.available()) {
    uint8_t b = Serial2.read();
    processByte(b); 
  }

  // Recepción de comandos por USB mediante líneas terminadas en '\n'.
  while (Serial.available()) {
    char ch = Serial.read();

    if (ch == '\n') {
      usbBuffer[usbIndex] = '\0';     
      handleUsbCommand(usbBuffer);   
      usbIndex = 0;                   
    } 
    else {
      if (usbIndex < MAX_USB_PAYLOAD - 1) {
        usbBuffer[usbIndex++] = ch;  // almacenar byte
      }
      // else: descarto
    }
  }

  // Recepción de comandos por Bluetooth mediante líneas terminadas en '\n'.
  while (SerialBT.available()) {
    char ch = SerialBT.read();

    if (ch == '\n') {
      btBuffer[btIndex] = '\0';
      handleBluetoothCommand(btBuffer);
      btIndex = 0;
    } else {
      if (btIndex < CMD_BUF_SIZE - 1) {
      btBuffer[btIndex++] = ch;
      }
    }
  }


  // Sincroniza las tareas periódicas con las interrupciones del temporizador
  // y programa la transmisión de telemetría por Bluetooth.

  if (xSemaphoreTake(timerSemaphore, 0) == pdTRUE) {    //entra una vez y se desactiva hasta que vuelva a haber otra interrupcion
    uint32_t isrCount = 0, isrTime = 0;
    // Read the interrupt count and time
    portENTER_CRITICAL(&timerMux);
    isrCount = isrCounter;
    isrTime = lastIsrAt;
    if (isrCounter > 3) {        //1 segundo
      isrCounter = 0;
    }
    portEXIT_CRITICAL(&timerMux);

    //Bluetooth
    if (isrCounter == 0 || isrCounter == 2) {    //medio segundo
      if (SerialBT.connected()) {
        BluetoothTX ();
      }
    }
  }

  // Actualización del tiempo de funcionamiento de la caminadora.
  updateTreadmillTime(td.speed_setpoint_u);

  // Actualización del reloj mostrado en pantalla.
  displayTime();

  // Lectura y gestión de los botones físicos.
  handleButtons();

}