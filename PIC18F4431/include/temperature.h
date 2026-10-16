
#ifndef TEMPERATURE_H
#define	TEMPERATURE_H

unsigned char ow_reset(void);
unsigned char read_bit(void);
unsigned char read_byte(void);
void write_bit(char bitval);
void write_byte(char val);
void FSMtemperatura(void);
void SensorTempInit(uint8_t);
float GetTemperature(void);

#endif	

