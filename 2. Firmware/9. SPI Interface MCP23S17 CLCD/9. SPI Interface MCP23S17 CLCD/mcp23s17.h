#ifndef MCP23S17_H_
#define MCP23S17_H_

// MCP23S17 IO EXPANDER REGISTERS
#define		IOX_ADR_WRITE	0x40 // 0b0100_0000
#define		IOX_ADR_READ	0x41 // 0b0100_0001
#define		IOX_IODIRA		0x00
#define		IOX_IODIRB		0x01
#define		IOX_IOCON		0x0a
#define		IOX_GPIOA		0x12
#define		IOX_GPIOB		0x13

extern uint8_t spi_txrx_buf[3];

void MCP23S17_Init(void);

void MCP23S17_WriteReg(uint8_t reg, uint8_t data);
uint8_t MCP23S17_ReadReg(uint8_t reg);

void MCP23S17_WriteGPIOA(uint8_t value);
void MCP23S17_WriteGPIOB(uint8_t value);

uint8_t MCP23S17_ReadGPIOA(void);
uint8_t MCP23S17_ReadGPIOB(void);

void MCP23S17_SetPinDirectionA(uint8_t pin, bool output);
void MCP23S17_SetPinDirectionB(uint8_t pin, bool output);

#endif /* MCP23S17_H_ */