
#ifndef PWM_H
#define	PWM_H

#include <xc.h>
#include "../include/config.h"
#include <stdio.h>
#include <stdint.h>
#include "pid.h"

#include <stdbool.h>

#include "../include/ADC1canal.h"
#include "../include/UART.h"

void PWM_Init(float);
//void enable_fault(void);

#endif	

