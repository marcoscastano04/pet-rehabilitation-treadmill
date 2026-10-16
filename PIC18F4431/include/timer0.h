
#ifndef timer0_H
#define	timer0_H

#include <xc.h> 

void tmr0_Init()
{
    TMR0H = 0;
    TMR0L = 0;
    T0CON = 0x88;  //enable, 16 bits, internal clock, 1:1, low to high
    TMR0IF = 0;    // reset timer0 interrupt flag 
    TMR0IE = 1;    //enable timer0 interrupt
    GIE = 1;       //enable global interrupt
    PEIE = 1;      //enable peripheral interrupts
    
    IPEN = 1;      //enable priority interrupts
    TMR0IP = 0;    //timer0 low priority
}

#endif	

