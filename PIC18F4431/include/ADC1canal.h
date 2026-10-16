
#ifndef ADC1canal_H
#define	ADC1canal_H

#include <xc.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>   //nose si es necesario poner las librerias

#define _XTAL_FREQ 40000000

//declaracion de funciones
void ADC_Init(void);
uint16_t ADC_Read(uint8_t);


#endif	

