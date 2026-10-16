#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include "globals.h"

// Bluetooth callbacks and helpers
void BTKeyRequestCallback(void);
void BTConfirmRequestCallback(uint32_t numVal);
void BTAuthCompleteCallback(boolean success);

// Manejo de comandos recibidos por Bluetooth (visible para el .ino)
void handleBluetoothCommand(const char *cmd);
void BluetoothTX (void);

#endif // BLUETOOTH_H