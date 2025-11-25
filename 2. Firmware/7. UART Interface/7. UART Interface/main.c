#define F_CPU 5000000UL

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdio.h>
#include <stdbool.h>

#include "uart.h"

void CLK_Init(void);
void TCB0_Init(void);

volatile bool flag = false;

int main(void)
{
	CLK_Init();
	TCB0_Init();
	
	USART0_Init(115200);
	
	sei();
	while (1)
	{
		if(flag)
		{
			flag = false;
			printf("Hello World!!\r\n");
		}
	}
}

void CLK_Init(void)
{
	CCP = CCP_IOREG_gc;
	CLKCTRL.MCLKCTRLB = CLKCTRL_PDIV_4X_gc | CLKCTRL_ENABLE_bm;
}

void TCB0_Init(void)
{
	TCB0.CCMP = 5000;
	TCB0.CTRLA |= TCB_ENABLE_bm;
	TCB0.INTCTRL |= TCB_CAPT_bm;
}

ISR(TCB0_INT_vect)
{
	static uint16_t Cnt1000Hz = 0;
	uint16_t Cnt1Hz = (uint16_t)(Cnt1000Hz % 1000);
	Cnt1000Hz++;
	
	if(Cnt1Hz == 0) flag = true;

	TCB0.INTFLAGS |= TCB_CAPT_bm;
}