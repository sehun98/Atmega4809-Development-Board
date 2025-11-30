#define F_CPU 5000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdbool.h>
#include "i2c.h"
#include "ds1621.h"


void DS1621_START_CONVERT_T(void)
{
	I2C_Write_Command(DS1621_SLAVE_ADDRESS, START_CONVERT_T);
}
uint16_t DS1621_READ_TEMPERATURE(void)
{
	return I2C_Read_Cmd_Uint16(DS1621_SLAVE_ADDRESS, READ_TEMPERATURE);
}

void DS1621_SetHighLimit(uint8_t HighLimit)
{
	I2C_Write_Cmd_Uint8( DS1621_SLAVE_ADDRESS, ACCESS_TH, HighLimit);
}

void DS1621_SetLowLimit(uint8_t LowLimit)
{
	I2C_Write_Cmd_Uint8( DS1621_SLAVE_ADDRESS, ACCESS_TL, LowLimit);
}

void DS1621_STOP_CONVERT_T(void)
{
	I2C_Write_Command(DS1621_SLAVE_ADDRESS, STOP_CONVERT_T);
}