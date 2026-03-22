#define F_CPU	5000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <stdio.h>

#include "dht11.h"
#include "uart.h"

void CLK_Init(void);

int main(void)
{
	uint8_t humdi = 0, temp = 0;

	CLK_Init();
	USART0_Init(115200);

	DHT11_StartTimer();
	DHT11_Init();
	sei();
	
	printf("DHT11 Start!\r\n");

	while (1)
	{
		if (DHT11_MeasureFlag)
		{
			DHT11_MeasureFlag = false;

			if (DHT11_Read(&humdi, &temp))
				printf("Humidity: %d, Temp: %d\r\n", humdi, temp);
			else
				printf("Checksum Error\r\n");
		}
	}
}

void CLK_Init(void)
{
	CCP = CCP_IOREG_gc;
	CLKCTRL.MCLKCTRLB = CLKCTRL_PDIV_4X_gc | CLKCTRL_ENABLE_bm;
}