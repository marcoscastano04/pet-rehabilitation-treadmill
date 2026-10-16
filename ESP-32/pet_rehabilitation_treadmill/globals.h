#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <BluetoothSerial.h>
#include <LiquidCrystal_I2C.h>

// Pins (para que estén disponibles en todos los .cpp)
#define BUTTON_UP_PIN   27
#define BUTTON_DOWN_PIN 26
#define BUTTON_FAST_PIN 12
#define BUTTON_SLOW_PIN 14

// Tamaños máximos de los búferes de comunicación.
#define MAX_RX_BUFFER 64
#define MAX_RX_PAYLOAD 38
#define MAX_TX_BUFFER 64
#define MAX_TX_PAYLOAD 32
#define CMD_BUF_SIZE 64
#define MAX_USB_PAYLOAD 32

// Constantes de capacidades bluetooth 
constexpr bool INPUT_CAPABILITY = true;   // Defines if ESP32 device has input method (Serial terminal, keyboard or similar)
constexpr bool OUTPUT_CAPABILITY = true;  // Defines if ESP32 device has output method (Serial terminal, display or similar)

// Structure de telemetria
typedef struct {
    float    speed_measured;
    uint16_t ic3_buffer;
    int16_t  speed_setpoint_u;
    int16_t  speed_setpoint_m;
    float    salida_y;
    float    error;
    float    current;
    float    acumulador_integral;
    float    bus_voltage;
    float    temperature;
    uint32_t t1_ticks;
} TelemetryData;

// Declaraciones de variables globales definidas en un único archivo fuente.
extern TelemetryData td;
extern TelemetryData td_prev;

// Declaraciones de objetos compartidos definidos en un único archivo fuente.
extern LiquidCrystal_I2C lcd;
extern BluetoothSerial SerialBT;

// Banderas/variables globales compartidas
extern bool PairingInProgress;
extern bool screenNeedsRefresh;
extern int currentScreen;
extern const int maxScreens;
extern bool PIC_to_UART;

// Bluetooth buffer (declaración)
extern char btBuffer[CMD_BUF_SIZE];
extern uint8_t btIndex;

// Funciones compartidas entre módulos (declaraciones)
void sendCommandSpaced(const char *cmd);
void processByte(uint8_t b);
void processPacket(uint8_t *d, uint8_t len);

// Funciones de display/telemetría usadas desde otros módulos
void checkTelemetryChanges(void);
void updateTreadmillTime(float speed);

extern const bool ButtonText;

// USB handler (si está definido en el .ino)
void handleUsbCommand(const char *cmd);

// Variables de tiempo utilizadas en la interfaz y la depuración.
extern uint16_t minutes;
extern uint16_t seconds;

#endif // GLOBALS_H