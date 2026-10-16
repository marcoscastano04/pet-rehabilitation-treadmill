#ifndef UART_H
#define	UART_H

#include <xc.h>
 
#define INPUT 1
#define OUTPUT 0 

#include <stdint.h>
#include <stdbool.h>
//#include <stdio.h>

// Functions Declarations

void UART_TX_Init(void);

/* API pública */
bool uart_start_frame(const uint8_t *data, uint8_t len);
void uart_tx_task(void);

/* RX API (new) */
void uart_register_command_callback(void (*cb)(char *cmd));
void uart_on_rx_interrupt(void); /* call from ISR when RCIF set */
void uart_rx_task(void);         /* call from main loop to process buffered bytes */

#endif	

