
#ifndef UART_PARSE_H
#define	UART_PARSE_H

#include <xc.h> // include processor files - each processor file is guarded.  

long parse_long(const char *s);
long parse_fixed(const char *s, uint8_t decimals);
uint16_t parse_uint16_clamped(const char *s);
int parse_percent(const char *s);

#endif	/* XC_HEADER_TEMPLATE_H */

