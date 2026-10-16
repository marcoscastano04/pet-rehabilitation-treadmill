#include <xc.h>
#include "../include/temperature.h"
#define DQ_PORT   PORTBbits.RB5
#define DQ_LAT    LATBbits.LATB5
#define DQ_TRIS   TRISBbits.TRISB5

#define INPUT 1
#define OUTPUT 0 

#define _XTAL_FREQ 40000000

//Comandos especiales del sensor
#define   skip_rom 0xCC
#define   convert_temp 0x44
#define   write_scratchpad 0x4E
#define   resolution_12bit 0x7F
#define   resolution_9bit  0x1F
#define   read_scratchpad 0xBE

//SEMIBLOQUEANTE: bloqueo en los readbyte y writebyte pero dejo que peocese acciones sin bloquear

enum {
    TEMP_IDLE,
    TEMP_RESET,
    TEMP_SKIP_ROM,
    TEMP_START_CONV,
    TEMP_WAIT_CONV,
    TEMP_READ_RESET,
    TEMP_READ_SKIP,
    TEMP_READ_CMD, 
    TEMP_READ_DATA,
    TEMP_PROCESS_DATA,
};

uint8_t tempState = TEMP_IDLE;
uint16_t tempCounter = 0;
static float Temperatura = 0;

uint8_t TempL;
uint8_t TempH;   
int16_t rawTemp; 

void FSMtemperatura(void)
{
    switch (tempState)
    {
        case TEMP_IDLE:
            // Esperar el intervalo entre mediciones.
            tempCounter++;
            if (tempCounter >= 80)         // 80 * 6.5ms = 524ms   
            {
                tempCounter = 0;
                tempState = TEMP_RESET;
            }
            break;                
            
        case TEMP_RESET:
            ow_reset();   // Reiniciar el bus OneWire e iniciar la detección del sensor.             
            //~1.02ms (sin contar las funciones atomicas)
            tempState = TEMP_SKIP_ROM;
            break;
            
        case TEMP_SKIP_ROM:
            write_byte(skip_rom); // Seleccionar el sensor sin utilizar su código ROM ~0.5ms
            tempState = TEMP_START_CONV;
            break;
            
        case TEMP_START_CONV:
            write_byte(convert_temp);   // Iniciar la conversión de temperatura ~0.5ms
            tempState = TEMP_WAIT_CONV;
            break;

        case TEMP_WAIT_CONV:               
            // Espera el período de conversión sin bloquear (375ms para 11bits)
            tempCounter++;
            if (tempCounter >= 58)          //58*6.55ms ~ 380ms
            {                               
                tempCounter = 0;
                tempState = TEMP_READ_RESET;
            }
            break;

        case TEMP_READ_RESET:
            ow_reset();        //~1.02ms
            tempState = TEMP_READ_SKIP;
            break;
            
        case TEMP_READ_SKIP:
            write_byte(skip_rom);   //~0.5ms
            tempState = TEMP_READ_CMD;
            break;
            
        case TEMP_READ_CMD:
            write_byte(read_scratchpad); // Solicitar la lectura de la memoria Scratchpad ~0.5ms
            //el sensor espera a que se lea el resto de los bytes pero se puede resetar
            tempState = TEMP_READ_DATA;
            break;
            
        case TEMP_READ_DATA:
            {
                TempL = read_byte();        //~0.5ms (480us)
                TempH = read_byte();        //~0.5ms (primeros bytes de scratch pad son la temperatura)   
            }
            tempState = TEMP_PROCESS_DATA;
            break;
            
        case TEMP_PROCESS_DATA:
            rawTemp = (TempH << 8) | TempL;
            Temperatura = rawTemp * 0.0625f;  
            tempState = TEMP_IDLE;
            break;
    }
}

void SensorTempInit(uint8_t resolution)     //Bloqueante
{
    DQ_TRIS = INPUT;
    ow_reset();                             //~1.02ms
    write_byte(write_scratchpad);           //~0.5ms
    write_byte(0);                          //salteo los bytes de seteo de alarma
    write_byte(0);
    write_byte(resolution);                 //byte de configuracion
}

float GetTemperature(void)
{
    return Temperatura;
}

unsigned char ow_reset(void)
{
    DQ_TRIS = OUTPUT; 
	DQ_LAT = 0;             //LOW por un minimo de 480us para la secuencia de inicialización
	__delay_us(480);        
	DQ_TRIS = INPUT;        // Liberar el bus y permitir la respuesta del sensor. 
	__delay_us(60);         
	if (DQ_PORT == 0)       // Detecta el pulso de presencia del sensor.
	{ 
		__delay_us(480);    //Sensor transmite por 60us a 240us pero el maestro tiene que recibir por 480us minimo
		return 0;           //Sensor presente
	} 
	else 
	{
		__delay_us(480);
		return 1;           //No se detectó presencia
	}
} 

unsigned char read_bit(void)
{
    unsigned char bitval;
    DQ_TRIS = OUTPUT; 
    DQ_LAT = 0;           // LOW por lo menos 1us
    __delay_us(2);       
    DQ_TRIS = INPUT;      // Suelta el bus
    __delay_us(13);       // Esperar 15us desde el start
    bitval = DQ_PORT;     // Read bit
    __delay_us(45);       // Esperar el resto del timeslot (~60us total)
    return bitval;
}

unsigned char read_byte(void)
{ 
    char i,result = 0;
    // Leer ocho bits, comenzando por el menos significativo.
    for(i = 0; i < 8; i++)
	{
		DQ_TRIS = OUTPUT; 
		DQ_LAT = 0;             
		__delay_us(2);
        
		DQ_TRIS = INPUT;    // Liberar el bus para recibir el bit.
        __delay_us(13);     
        
		if(DQ_PORT != 0) 
            result |= 1<<i;
        
		__delay_us(45);     // Completar el intervalo de lectura.
	}
	return result;
}

void write_bit(char bitval)
{
	DQ_TRIS = OUTPUT;
    DQ_LAT = 0;             // Llevar DQ a nivel bajo para iniciar el intervalo de transmisión.
    __delay_us(2);
    if(bitval) {
        __delay_us(5);       // Mantener el nivel bajo para transmitir un '1'.
        DQ_TRIS = INPUT;     // Liberar la línea para que la resistencia pull-up suba el nivel del bus.
        __delay_us(55);      // Completar el resto del intervalo de transmisión
    } else {
        __delay_us(60);      // Mantener el nivel bajo para transmitir un '0'.
        DQ_TRIS = INPUT;     // Liberar la línea.
        __delay_us(5);       
    }
}

void write_byte(char val)
{
    char i;
    for(i = 0; i < 8; i++)
    {
        DQ_TRIS = OUTPUT; 
        DQ_LAT = 0; 
        __delay_us(2);             
        if((val & (1<<i))!= 0) 
        {
        // write 1
        __delay_us(5);              
        DQ_TRIS = INPUT;            
        __delay_us(55);           
        } 
        else 
        {
        //write 0
        __delay_us(60);            
        DQ_TRIS = INPUT;          
        }
    }
}


//-------------------------------------------------------------------------------------------


//NO BLOQUEANTE PURO: pero necesita de timer1 con interrupciones cada 60us

//volatile uint8_t ow_state = 0;
//volatile uint16_t ow_timer = 0;
//volatile uint8_t ow_bit_value;
//
//#define OW_PIN PORTBbits.RB0
//#define OW_TRIS TRISBbits.TRISB0
//
//enum {
//    OW_IDLE,
//    OW_RESET_LOW,
//    OW_WAIT_PRESENCE,
//    OW_READ_PRESENCE,
//    OW_WRITE_BIT_START,
//    OW_WRITE_BIT_END,
//    OW_READ_BIT_START,
//    OW_READ_BIT_SAMPLE,
//};
//
//void ow_tick(void)
//{
//    switch (ow_state)
//    {
//        case OW_IDLE:
//            break;
//
//        case OW_RESET_LOW:
//            OW_TRIS = 0;   // salida
//            OW_PIN  = 0;   // tirar abajo
//            if (++ow_timer >= 80)   // 80 * 60us ? 480us
//            {
//                ow_timer = 0;
//                OW_TRIS = 1;    // soltar el bus
//                ow_state = OW_WAIT_PRESENCE;
//            }
//            break;
//
//        case OW_WAIT_PRESENCE:
//            // esperar 60 us antes de leer
//            if (++ow_timer >= 1)   // 1 tick = 60us
//            {
//                ow_timer = 0;
//                ow_state = OW_READ_PRESENCE;
//            }
//            break;
//
//        case OW_READ_PRESENCE:
//            if (OW_PIN == 0)
//                presence_detected = 1;
//            else
//                presence_detected = 0;
//
//            ow_state = OW_IDLE;
//            break;
//
//        case OW_WRITE_BIT_START:
//            Ow_TRIS = 0;          // salida
//            OW_PIN  = 0;          // llevar a 0
//            if (ow_bit_value)     // si es '1', soltar rápido
//            {
//                if (++ow_timer >= 1)    // 60us
//                {
//                    OW_TRIS = 1;
//                    ow_state = OW_WRITE_BIT_END;
//                    ow_timer = 0;
//                }
//            }
//            else                 // si es '0', mantener más tiempo
//            {
//                if (++ow_timer >= 10)   // 10*60us=600us
//                {
//                    OW_TRIS = 1;
//                    ow_state = OW_WRITE_BIT_END;
//                    ow_timer = 0;
//                }
//            }
//            break;
//
//        case OW_WRITE_BIT_END:
//            // esperar 1 tick para finalizar el slot
//            if (++ow_timer >= 1)
//            {
//                ow_timer = 0;
//                ow_state = OW_IDLE;
//            }
//            break;
//
//        case OW_READ_BIT_START:
//            OW_TRIS = 0;
//            OW_PIN = 0;
//            if (++ow_timer >= 1)   // 60us
//            {
//                OW_TRIS = 1;       // soltar línea
//                ow_state = OW_READ_BIT_SAMPLE;
//                ow_timer = 0;
//            }
//            break;
//
//        case OW_READ_BIT_SAMPLE:
//            // samplear a los 15us ? como no tenemos 15us exactos,
//            // sampleamos al siguiente tick (aprox)
//            ow_bit_value = OW_PIN;
//            ow_state = OW_IDLE;
//            break;
//    }
//}