/*
 * File:   main.c
 * Author: m_cas
 *
 * Created on 3 de octubre de 2024, 17:38
 */


#include <xc.h>
#include <string.h>
#include <stdio.h>

#include "../include/config.h"
#include "../include/ADC1canal.h"
#include "../include/MFM.h"
#include "../include/timer0.h"
#include "../include/timer1.h"
#include "../include/PWM.h"
#include "../include/pid.h"

#include "../include/UART.h"
#include "../include/protocol.h"
#include "../include/temperature.h"
#include "../include/uart_parse.h"
#include "../include/safety.h"


#define INPUT 1
#define OUTPUT 0 

#define W_MAX 358
#define W_RES 4
#define W_MIN 26

#define FREQ_PWM 19600

//BANDERAS
    volatile uint8_t flag_timer0 = 0;
    volatile uint8_t flagIC3 = 0;
    volatile uint8_t flagPWM = 1;
    volatile uint8_t flagF = 1;
    volatile uint8_t flagWrite = 1;
    volatile uint8_t flagStart = 0; 
    

//CONSTANTES Y VARIABLES
    //Velocidad
    const uint16_t Wmax = W_MAX;
    const uint16_t Wmin = W_MIN;
    uint16_t IC3buffer;
    volatile uint8_t overflow5 = 0;
    volatile uint8_t overflow5_old = 0;
    float w;
    long WspTemp;
    uint16_t CountToStop;
    
    //Tension
    uint16_t Nadc7;
    float Vbus;
    
    //Corriente
    uint16_t Nadc0;
    float Vadc;
    float Ia;
    float const Vrefmenos = 1.3889;
    float const ResAN0 = 0.0021492;
    uint16_t contI = 0;
    
    uint16_t Nadc1;
    float Ia1;
    float Vadc1;

    //PWM
    uint16_t Fpwm = FREQ_PWM; 
    long aux;
    long PWMTemp;
    
    uint16_t PWMPeriod;
    
    uint16_t ciclo;
    uint16_t dutyCycle;
    
    //timer1
    uint8_t overflow1_count;
    uint16_t t1_start, t1_start_new, t1_end;
    
//CONSTANTES Y VARIABLES DEL PID
    float Ki = 1.0;   
    float Kp = 1.6;
    
    float e = 0;
    float e_old = 0;
    float ye = 0;
    
    float acumuladorI = 0;
    const float Ts = 0.0065536;
    uint8_t flagClamp = 0;
    
//CONSTANTES Y VARIABLES DEL PRE-FILTRO
    uint16_t WspU = 0;
    uint16_t WspM = 0;
    int contF = 0;
    int itF = 7;

//UART

    const char *arg = NULL; 
    uint16_t FpwmNEW;
    long WspTemp;
    int p;
    long KpTemp;
    long KiTemp;
    
    //TRANSMISION
    tx_frame_t tx_frame;
    volatile uint8_t cont_new_tx = 0;
    
//------------------------------------------------------------------------------------------
    
// Declaración anticipada del manejador de comandos UART./
static void uart_command_handler(char *uart_buffer);

// Procesa los comandos recibidos por UART desde el contexto principal.
static void uart_command_handler(char *rx_cmd)
{
    switch (rx_cmd[0])
    {
        case 'g':                   // Subir velocidad
            if (WspU == 0) {
                WspU = Wmin;
            } else if (WspU >= Wmax) {
                WspU = Wmax;
            } else {
                WspU += W_RES;
            }
            rx_cmd[0] = '0';
            break;

        case 's':                  // Bajar velocidad
            if (WspU <= Wmin) {
                WspU = 0;
            } else {
                WspU -= W_RES;
            }
            rx_cmd[0] = '0';
            break;

        case '0':                  // Default state
            break;
            
         case 'k':      // Modificar las ganancias del controlador PI.
            if (rx_cmd[1] =='p'){
                arg = &rx_cmd[2];
                if (*arg == '=' || *arg == ' ') arg++;
                KpTemp = parse_fixed(arg, 0);   // scaled by 100
                Kp = (float)KpTemp / 100.0f;
            } else if (rx_cmd[1] == 'i') {
                arg = &rx_cmd[2];
                if (*arg == '=' || *arg == ' ') arg++;
                KiTemp = parse_fixed(arg, 0);
                Ki = (float)KiTemp / 100.0f;
            }
             rx_cmd[0] = '0';
             break;

        case 'w':        // Habilitar o deshabilitar la transmisión de telemetría.
            if (rx_cmd[1] == 'r'){
                flagWrite = !flagWrite;
            }
            rx_cmd[0] = '0';
            break;

        case 'f':      // Modificar la frecuencia de PWM.
            if (rx_cmd[1] =='p'){
                arg = &rx_cmd[2];
                if (*arg == '=' || *arg == ' ') arg++;
                FpwmNEW = parse_uint16_clamped(arg);
                if ((FpwmNEW > 0 ) && (FpwmNEW <= 40000)){
                    Fpwm = FpwmNEW;
                }
            }
            rx_cmd[0] = '0';
//            PWM_Init(Fpwm);
            break;

        case 'd':      // Establecer manualmente el ciclo de trabajo de PWM.
            if (rx_cmd[1] =='t'){
                arg = &rx_cmd[2];
                if (*arg == '=' || *arg == ' ') arg++;
                p = parse_percent(arg);
                if ((p >= 0) && (p <= 100)) {
                    flagPWM = 2;
                    PWMTemp = p;
                } else {
                // valores fuera del intervalo permitido se ignoran
                }
            }
            rx_cmd[0] = '0';
            break;

        case 'v':    // Establecer la referencia de velocidad mediante un porcentaje.
            if (rx_cmd[1] =='t'){
                arg = &rx_cmd[2];
                if (*arg == '=' || *arg == ' ') arg++;
                WspTemp = parse_percent(arg);
                if (WspTemp >= 0) {
                    WspU = (uint16_t)((uint32_t)WspTemp * Wmax / 100);
                }
            }
            rx_cmd[0] = '0';
            break;

        case 'r':                 //Toggle PWM
            if (rx_cmd[1] =='t'){
                flagPWM = !flagPWM ;
            }
            rx_cmd[0] = '0';
            break;

        case 'p':                 //Toggle pre-filtro
            if (rx_cmd[1] =='f')  
            {
                flagF = !flagF;  
            } 
            rx_cmd[0] = '0';
            break;

        default:
            rx_cmd[0] = '0';
            break;
    }
}
    
//------------------------------------------------------------------------------------------
    
// Atender las interrupciones de los temporizadores y del módulo de captura.
void __interrupt(low_priority) interru_timer0() {
    if (TMR0IF == 1) 
    {
        flag_timer0 = 1;
        TMR0IF = 0;
    }

    if (TMR5IF == 1) 
    {
        if (overflow5 < 255) 
        {
            overflow5++;
        }
        TMR5IF = 0;
    }

    if (IC3DRIF == 1) 
    {
        flagIC3 = 1;
        IC3DRIF = 0;
    }
    
    if (TMR1IF)
    {
        TMR1IF = 0;
        overflow1_count++;
    }

}
// Atender la recepción UART.
void __interrupt(high_priority) interrupciones() {
    if (RCIF)    
    {
        uart_on_rx_interrupt();
        LATD1 = ~LATD1;         //LED
    }
}  



void main(void) {
    ADC_Init();
    MFM_Init();
    PWM_Init(Fpwm);
    tmr0_Init();
    tmr1_Init();
    UART_TX_Init();
    SensorTempInit(resolution_11bit);
    
    // Registrar el callback que uart.c invocará al completar la recepción de una trama.
    uart_register_command_callback(uart_command_handler);
    
    LATD1 = 0;
    TRISD1 = OUTPUT;   //LED de comunicación
    LATD0 = 1;
    TRISD0 = OUTPUT;   //LED stand-by
    LATD2 = 0;
    TRISD2 = OUTPUT;   //safe start


    while (1) {
        
        //--------------------------------------------------
        //              MUESTREO TIMER0
        //--------------------------------------------------
        if (flag_timer0 > 0) 
        {
            // Registrar el instante inicial para medir el tiempo de ejecución.
            t1_start_new = ((uint16_t)TMR1H << 8) | TMR1L;
            
            //SAFE START
            if (flagStart == 0){
                flagStart = Safe_Start();
            }
            else{
                 LATD2 = 1;
            }
            
            //VELOCIDAD
            if (flagIC3 == 1)
            {
                overflow5_old = overflow5;
                overflow5 = 0;
                flagIC3 = 0;

                if (overflow5_old < 4)
                {
                    IC3buffer = (uint16_t) CAP3BUFH<<8 | CAP3BUFL;
                    if ((IC3buffer != 0) || (overflow5_old != 0))  // Evitar la división por cero antes de recibir una captura válida.                                 
                    {
                        uint32_t aux = (uint32_t)overflow5_old*65536UL + IC3buffer;
                        w = 981750/(float)aux;
                    }
                }
                else 
                {
                    w = 0;
                    // Reiniciar los buffers para la proxima marcha.
                    CAP3BUFH = 0;  
                    CAP3BUFL = 0;   
                }
            }
            else if (overflow5 >= 4)
            {
                w = 0;
                CAP3BUFH = 0;  
                CAP3BUFL = 0;
            }
            
            //CONTROL
            if (flagPWM == 1) 
            {
                
                //PRE-FILTRO
                if (flagF == 1)
                {
                    if (WspU > WspM) 
                    {
                        contF++;
                    } 
                    else if (WspU < WspM)
                    {
                        contF--;
                    }
                    
                    if (contF > itF)  
                    {  //8 iteraciones para 5 segundos en ir de 0 a 25% vel max
                        contF=0;
                        WspM++;
                    }
                    else if (contF < -itF)
                    {
                        contF=0;
                        WspM--;
                    }   
                }
                else
                {
                    WspM = WspU; 
                }
            
                //COMPENSADOR
                
                // ERROR
                e = WspM - w;

                //Cálculo de la salida sin saturar y con el acumulador desactualizado
                float ye_unsat = Kp*e + Ki*acumuladorI;

                //Aplicar saturacion
                float ye_sat = ye_unsat;

                if (ye_sat > Wmax) ye_sat = Wmax;
                if (ye_sat < 0)    ye_sat = 0;

                // Anti-windup 
                if (
                    (ye_sat < Wmax || e < 0) &&  // no saturado arriba o error ayuda a bajar
                    (ye_sat > 0    || e > 0)     // no saturado abajo o error ayuda a subir
                   )
                {
                    acumuladorI = acumuladorI + Ts*(e+e_old)/2;
                }

                // Recalcular salida con integrador actualizado
                ye = Kp*e + Ki*acumuladorI;

                // Saturacion final 
                if (ye > Wmax) ye = Wmax;
                if (ye < 0)    ye = 0;
                
                //Guardar el valor de error
                e_old = e;
                
                //cuando esta andando, y vuelvo a cero, limpio el acumulador
                if (WspM == 0)
                {
                    ye = 0;
                    acumuladorI = 0;
                }
                
                //PWM
                float pre = ye/Wmax;
                float dutyCycle1 = PWMPeriod*pre;

                dutyCycle = dutyCycle1;
                uint16_t dutyCycleQC=dutyCycle*4;  //Agregar dos ceros que equivalen al ciclo de clock dentro de la instruccion.
                PDC0H=dutyCycleQC/256;
                PDC0L=dutyCycleQC%256;
            }
            else if (flagPWM == 2)
                {

                    float pre = (float)PWMTemp/100;
                    float dutyCycle1= PWMPeriod*pre;
                    uint16_t dutyCycle = dutyCycle1;
                    uint16_t dutyCycleQC=dutyCycle*4; 
                    PDC0H=dutyCycleQC/256;
                    PDC0L=dutyCycleQC%256;
                }
            else 
            {
                PDC0H = 0;
                PDC0L = 0;
                e = 0;
                e_old = 0;
                acumuladorI = 0;
                WspU = 0;
                WspM = 0;
                contF = 0;
            }
            
            //CORRIENTE
            Nadc0 = ADC_Read(1);
            Vadc = Nadc0*ResAN0+Vrefmenos;
            Ia = Vadc/0.185-13.5135;
                      
            //TENSION
            uint16_t Nadc7 = ADC_Read(7);
            Vbus = 0.1664*Nadc7;        //divisor resistivo*5v/1024
            
            //Cálculo del tiempo que tardo la iteracion anterior
            int32_t total_ticks = ((int32_t)overflow1_count << 16) + ((int32_t)t1_end - (int32_t)t1_start);
            overflow1_count = 0;


            //TRANSMISION: copia de datos a la maquina de estados

            if (flagWrite == 1)
            { 
                cont_new_tx++;
                if (cont_new_tx > 37)     //250ms
                {
                    tx_frame.speed_measured = w;
                    tx_frame.ic3_buffer = IC3buffer;
                    tx_frame.speed_setpoint_u = WspU;
                    tx_frame.speed_setpoint_m = WspM;
                    tx_frame.salida_y = ye;
                    tx_frame.error = e;
                    tx_frame.current = Ia;
                    tx_frame.acumulador_integral = acumuladorI;
                    tx_frame.bus_voltage = Vbus;
                    tx_frame.temperature = GetTemperature();
                    tx_frame.t1_ticks = total_ticks;

                    uart_start_frame((uint8_t *)&tx_frame,sizeof(tx_frame));

                    cont_new_tx = 0;
                }
            }
            FSMtemperatura();
            t1_start = t1_start_new;   //Guarda el start para calcular el tiempo en la proxima iteración
            t1_end = ((uint16_t)TMR1H << 8) | TMR1L;    //Termina de contar el temporizador 1
            flag_timer0 = 0;   
        }
        uart_tx_task();
        uart_rx_task();      
    }
}
