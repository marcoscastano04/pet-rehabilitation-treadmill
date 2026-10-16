#include "../include/PWM.h"
extern uint16_t PWMPeriod;

void PWM_Init(float freq)
{
    PTCON0 = 0x00;   // Postescalador y preescalador 1:1; modo de funcionamiento continuo.
    PTCON1 = 0x80;   // Habilitar la base de tiempo y contar en sentido ascendente.
    PWMCON0 = 0x18;  // Habilitar únicamente PWM0, en modo independiente.
    
    unsigned long K_ptper; 
    
    K_ptper = 10000000;

    PWMPeriod = K_ptper/freq;
    PWMPeriod -= 1;
	PTPERH = PWMPeriod/256; // Escribir los bytes alto y bajo del período PWM.
	PTPERL = PWMPeriod%256;
    
}
