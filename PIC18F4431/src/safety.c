#include "../include/safety.h"
#include "../include/protocol.h"

static float Ia;
static uint16_t contI;
static uint8_t start_count = 0;

extern volatile uint8_t flagPWM ;

uint8_t Safe_Start(void){
    if (start_count <= 15){
    start_count++;
    }
    else if (start_count > 15){    //alrededoer de 100ms
        return 1;
    }
    return 0;
}
