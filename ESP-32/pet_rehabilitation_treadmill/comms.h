#ifndef COMMS_H
#define COMMS_H

#include <Arduino.h>

void processByte(uint8_t b);
void processPacket(uint8_t *d, uint8_t len);
void sendCommandSpaced(const char *cmd);

// Inicializa timers/estructuras internas necesarias para la transmisión espaciada
void initCommsTimers(void);

void handleUsbCommand(const char *cmd);

#endif // COMMS_H