
#ifndef timer1_H
#define	timer1_H

#include <xc.h> 

void tmr1_Init()
{
    TMR1H = 0;
    TMR1L = 0;
    T1CON = 0x81;  //16 bits, internal clock, 1:1, enabled
    
    TMR1IF = 0;    // reset timer0 interrupt flag 
    TMR1IE = 1;    //enable timer0 interrupt

    TMR1IP = 0;    //timer0 low priority
}

#endif	