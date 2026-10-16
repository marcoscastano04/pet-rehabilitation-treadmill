#include "../include/MFM.h"

void MFM_Init()
{
    // Configuración del módulo de captura CAP3.
    LATA4 = 0;
    TRISA4 = 1;
    ANSEL0bits.ANS4 = 0;  // Configurar AN4(cap3)como entrada digital. 
    
    QEICONbits.QEIM = 0;  // Deshabilitar el codificador en cuadratura.
    CAP3CON = 0x42;       // Captura en flanco ascendente con reinicio
    DFLTCON = 0x00;       // Deshabilitar inicialmente el filtro digital.
    
    IC3DRIP = 0;          // Interrupción de baja prioridad.
    IC3DRIF = 0;          // Limpiar la bandera de interrupción.
    IC3DRIE = 1;          // Habilitar la interrupción del IC3.
    
    // Configuración del Timer5 como base de tiempo para las capturas.
    T5CON = 0x59;         // Temporizador habilitado, prescaler 1:8.
    
    TMR5IP = 0;           // Interrupción de baja prioridad.
    TMR5IF = 0;           // Limpiar la bandera de interrupción.
    TMR5IE = 1;           // Habilitar la interrupción del Timer5.
    
    DFLTCONbits.FLT3EN = 1;
    DFLTCONbits.FLTCK = 0b011;    //1:16 
    
    CAP3BUFH = 0;
    CAP3BUFL = 0;
}
    
