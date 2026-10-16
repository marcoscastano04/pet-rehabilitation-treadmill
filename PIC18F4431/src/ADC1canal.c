
#include "../include/ADC1canal.h"

#define INPUT 1
#define OUTPUT 0 

void ADC_Init()
{
    ADCON0 = 0x04;  // Conversión de disparo único; autoconversion; mmuestreo y conversion de grupo A; ADC inicialmente apagado.
    ADCON1 = 0xC8;  // Referencias de tensión externas y FIFO deshabilitada.
    ADCON2 = 0x8A;  // Resultado alineado a la derecha; adquisición de 2 Tad y reloj Fosc/32.
    ADCON3 = 0xC0;  // Interrupciones del ADC deshabilitadas.
    
    ADCHS = 0x44;   //Selección de canales en cada grupo 
                    //(grupo D=AN7; grupo C= AN6; grupo A=AN0; grupo B=AN1)
    
    TRISA0 = INPUT; //AN0 (grupo A) establecerlo entrada analigica al tiristor
    ANS0 = 1;       //Establecerlo como entrada analogica para el modulo ADC
    
    TRISA1 = INPUT;   //AN1 (grupo B)
    ANS1 = 1;       

//    TRISE0 = INPUT;   //AN6  (grupo C)
//    ANS6 = 1;         
    
    TRISE1 = INPUT;   //AN7    (grupo D)
    ANS7 = 1;
    
    ADON = 1;         //Enciende el ADC
    __delay_us(10);   //Esperar 10us para que el módulo se estabilice.
}

uint16_t ADC_Read(uint8_t AN)
{
    uint16_t Conver ;
//    if (AN == 1)
//    {
//        ADCON0bits.ACMOD = 1;         //selecciono grupo D
//    }
    if (AN == 7)
    {
        ADCON1bits.VCFG = 0;          // Seleccionar VDD y VSS como referencias.
        ADCON2bits.ACQT = 0b0101;     // Configurar el tiempo de adquisición a 10*Tad.
        ADCON0bits.ACMOD = 3;         // Seleccionar el grupo D.
    }
   
    GO_nDONE = 1;                     // Iniciar la conversión ADC.
    while (GO_nDONE == 1);            // Esperar a que finalice.
    Conver = (uint16_t)ADRESH<<8 | ADRESL; // Combinar los registros del resultado.
    
    if (AN == 7)                
    {
        ADCON1bits.VCFG = 3;          // Restaurar las referencias de tensión externas.
        ADCON2bits.ACQT = 0b0101;     // Mantener el tiempo de adquisición configurado.
        ADCON0bits.ACMOD = 1;         // Volver al grupo A.
    }
//    if (AN == 1)
//    {
//        ADCON0bits.ACMOD = 0;         //selecciono grupo D
//    }
    return Conver;               
}
