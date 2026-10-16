#ifndef pid_H
#define	pid_H

#include <xc.h> // include processor files - each processor file is guarded.  

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>


//void pid_Init(void);
float pid_Set(int, float);

#endif	
