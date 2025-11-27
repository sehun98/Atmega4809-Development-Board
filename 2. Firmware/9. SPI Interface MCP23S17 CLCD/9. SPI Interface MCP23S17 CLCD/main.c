#define F_CPU 5000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "spi.h"
#include "uart.h"
#include "mcp23s17.h"
#include "clcd_mcp23s17.h"

void CLK_Init(void);

int main(void)
{
	CLK_Init();
	USART0_Init(115200);
	
    SPI_Init();

    sei();
	
	MCP23S17_Init();
	IOX_CLCD_Init();
	
	IOX_CLCD_DisplayString(0, 0, "Start SPI");
	
	_delay_ms(1000);
	IOX_CLCD_Clear();
    while (1)
    {

    }
    return 0;
}

void CLK_Init(void)
{
	CCP = CCP_IOREG_gc;
	CLKCTRL.MCLKCTRLB = CLKCTRL_PDIV_4X_gc | CLKCTRL_ENABLE_bm;
}