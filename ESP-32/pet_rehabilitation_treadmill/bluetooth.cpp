#include "bluetooth.h"
#include "globals.h"
#include <BluetoothSerial.h>
#include <string.h>

// Verificar la disponibilidad de Bluetooth según la configuración del ESP32.
#ifdef ARDUINO_ARCH_ESP32

  // Advertir si no están habilitadas las funciones necesarias para Bluetooth SPP.
  #if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
    #warning "CONFIG_BT_ENABLED or CONFIG_BLUEDROID_ENABLED not defined; Bluetooth may not work or requires enabling in menuconfig."
  #endif

  #if !defined(CONFIG_BT_SPP_ENABLED)
    #warning "CONFIG_BT_SPP_ENABLED not defined; Serial Port Profile (SPP) may not be available."
  #endif

#else
  #warning "Not compiling for ESP32 core/IDF; skipping CONFIG_* compile-time checks."
#endif

// Instancia global para la comunicación Bluetooth.
BluetoothSerial SerialBT;   
bool confirmRequestDone = false;
bool PairingInProgress = false;

// Búfer de recepción de comandos Bluetooth. (usado desde el .ino)
char btBuffer[CMD_BUF_SIZE];
uint8_t btIndex = 0;

// Gestionar la solicitud de confirmación del emparejamiento Bluetooth.
void BTConfirmRequestCallback(uint32_t numVal) {
  PairingInProgress = true;
#ifndef AUTO_PAIR
  Serial.printf("The PIN is: %06lu\nPress YES or NO button to confirm pairing:\n", numVal);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("PIN: "); lcd.print(numVal);
#else
  SerialBT.confirmReply(true);
#endif
}

void BTKeyRequestCallback() {
  Serial.println("BTKeyRequestCallback");  // debug
  char buffer[7] = {0};                    // 6 bytes de numeros y terminacion en '0'
  while (1) {
    Serial.print("Enter the passkey displayed on the other device: ");
    while (!Serial.available()) {
      delay(1);  // Feed the watchdog
      // Wait until data is available on the Serial port.
    }
    size_t len = Serial.readBytesUntil('\n', buffer, sizeof(buffer) - 1);
    buffer[len] = '\0';  // Null-terminate the string.
    try {
      uint32_t passkey = std::stoi(buffer);
      Serial.printf("Entered PIN: %lu\n", passkey);
      SerialBT.respondPasskey(passkey);
      return;
    } catch (...) {
      Serial.print("Wrong PIN! Try again.");
    }  
  }  
}

void BTAuthCompleteCallback(boolean success) {
  if (success) {
    Serial.println("Pairing success.");
  } else {
    Serial.println("Pairing failed, rejected by user.");
  }
}

void BluetoothTX (){
  float velKMsp = td.speed_setpoint_u*0.025f;
  float velKMmeasured = td.speed_measured*0.025f;
    String data_updateBT =
      String(velKMsp, 1) + "," +
      String(velKMmeasured, 1) + "," +
      String(td.temperature, 1) + "," +
      String(td.current, 1) + "," +
      String(td.bus_voltage, 1) + "," +
      String(td.salida_y, 1) + "," +
      String(td.ic3_buffer) + "," +
      String(td.error, 1) + "," +
      String(td.acumulador_integral, 1);
   SerialBT.print(data_updateBT);
}

void handleBluetoothCommand(const char *cmd) {
  if (cmd == NULL || cmd[0] == '\0') return;

  size_t len = strlen(cmd);

  // Validar los delimitadores del mensaje.
  if (len < 3 || cmd[0] != '<' || cmd[len - 1] != '>') {
    SerialBT.println("Invalid message format");
    return;
  }

  // Extraer el comando sin los delimitadores.
  char command[64];
  size_t commandLen = len - 2;

  if (commandLen >= sizeof(command)) {
    SerialBT.println("Command too long");
    return;
  }

  memcpy(command, cmd + 1, commandLen);
  command[commandLen] = '\0';

  char startChar = tolower((unsigned char)command[0]);

  // Reenviar a la PIC los comandos específicos de su firmware.
  if (startChar == 'g' || startChar == 's' || startChar == 'i' ||
      startChar == 'p' || startChar == 'q' || startChar == 'w' ||
      startChar == 'f') {
    Serial.print("Forwarding Bluetooth command to PIC: ");
    Serial.println(command);
    sendCommandSpaced(command);
    return;
  }

  // Procesar los comandos propios de la aplicación móvil.
  if (strcmp(command, "help") == 0) {
    SerialBT.println(
      "Commands:\n  g/s/i/p/q/w/f -> PIC\n  help -> show commands"
    );
  } else {
    SerialBT.print("Unknown command: ");
    SerialBT.println(command);
  }
}
