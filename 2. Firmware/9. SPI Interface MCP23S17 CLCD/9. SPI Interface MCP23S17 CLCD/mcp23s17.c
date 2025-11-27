#define F_CPU 5000000UL

#include "spi.h"
#include "mcp23s17.h"

#include <util/delay.h>

uint8_t spi_txrx_buf[3] = { 0 };

void MCP23S17_Init(void) 
{
	// 0b0011_0000
	// SEQOP Disable, DISSLW Enable 
	MCP23S17_WriteReg(IOX_IOCON, 0x30);
	
	// GPA0 : Input(1), GPA1-7 : Output(0)
	MCP23S17_WriteReg(IOX_IODIRA, 0x01);
	MCP23S17_WriteReg(IOX_GPIOA, 0x00);
	
	// KEY
	// GPB0-3 : Input, GPB4-7 : Output
	//MCP23S17_WriteReg(IOX_IODIRB, 0x0f);
}

void MCP23S17_WriteReg(uint8_t reg, uint8_t data)
{
	spi_txrx_buf[0] = IOX_ADR_WRITE;
	spi_txrx_buf[1] = reg;
	spi_txrx_buf[2] = data;
	SPI_Block_ReadWriteStart(spi_txrx_buf, 3);
	_delay_ms(1);
}

uint8_t MCP23S17_ReadReg(uint8_t reg)
{
	spi_txrx_buf[0] = IOX_ADR_READ;
	spi_txrx_buf[1] = reg;
	spi_txrx_buf[2] = 0xFF;
	SPI_Block_ReadWriteStart(spi_txrx_buf, 3);
	_delay_ms(1);
	
	return spi_txrx_buf[2];
}

void MCP23S17_WriteGPIOA(uint8_t value)
{
	MCP23S17_WriteReg(IOX_GPIOA, value);
}

void MCP23S17_WriteGPIOB(uint8_t value)
{
	MCP23S17_WriteReg(IOX_GPIOB, value);
}

uint8_t MCP23S17_ReadGPIOA(void)
{
	return MCP23S17_ReadReg(IOX_GPIOA);
}

uint8_t MCP23S17_ReadGPIOB(void)
{
	return MCP23S17_ReadReg(IOX_GPIOB);
}

void MCP23S17_SetPinDirectionA(uint8_t pin, bool output)
{
	uint8_t dir = MCP23S17_ReadReg(IOX_IODIRA);

	if (output)
	dir &= ~(1 << pin);  // 0 = Output
	else
	dir |= (1 << pin);   // 1 = Input

	MCP23S17_WriteReg(IOX_IODIRA, dir);
}

void MCP23S17_SetPinDirectionB(uint8_t pin, bool output)
{
	uint8_t dir = MCP23S17_ReadReg(IOX_IODIRB);

	if (output)
	dir &= ~(1 << pin);  // 0 = Output
	else
	dir |= (1 << pin);   // 1 = Input

	MCP23S17_WriteReg(IOX_IODIRB, dir);
}