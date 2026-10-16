#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

/* Frame TX PIC ? ESP32 */
typedef struct __attribute__((packed)) {
    float   speed_measured;   
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
} tx_frame_t;

#define UART_TX_PAYLOAD_LEN  (sizeof(tx_frame_t))

//TEMPERATURA
#define resolution_11bit 0x5F

#endif
