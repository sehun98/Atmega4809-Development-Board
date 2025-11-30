/*
 * #DS1621 #TemperatureCalculation
 * 본 장에서는 2-Wire(I2C) 기반의 디지털 온도센서로, 명령(Command Byte)을 통해 동작을 제어한다.
 * DS1621는 내부적으로 "디지털 카운터 방식(Digital Counter Method)"을 사용하여
 * 온도를 측정한다. 따라서 온도 데이터는 두 가지 단계로 구성된다.
 *
 * 1) 정수부(Integer Part)
 *    READ_TEMPERATURE(0xAA)의 High Byte에서 제공된다.
 *    예) High = 0x19 → 25°C
 *
 * 2) 소수점(Fractional Part)
 *    Low Byte의 bit7이 1이면 +0.5°C
 *    즉, 기본 해상도는 정수 + 0.5°C까지 표현 가능하다.
 *
 *    예)
 *      0x19 0x00 → 25.0°C
 *      0x19 0x80 → 25.5°C
 *
 * -----------------------------------------------------------
 * #정밀도 향상 (READ_COUNTER + READ_SLOPE)
 *
 * DS1621는 보다 정밀한 fractional 계산을 위해 두 개의 보정 값을 제공한다.
 *
 *   READ_COUNTER (0xA8) : Count_Remaining
 *   READ_SLOPE   (0xA9) : Slope Accumulator (센서 개체별 보정)
 *
 * 이 두 값을 사용하면 다음 공식으로 소수점을 더 정확하게 계산할 수 있다.
 *
 *   Temperature =
 *       TempRead
 *       - 0.25
 *       + (Slope - Count_Remaining) / Slope
 *
 * 이 방법을 사용하면 0.01°C ~ 0.1°C 단위까지 정밀도가 향상된다.
 *
 * callback)
 * 일반적인 애플리케이션은 READ_TEMPERATURE로 충분하지만,
 * 고정밀 측정이 필요한 경우에는 Counter/Slope 기반 계산을 적용한다.
 */


#define F_CPU 5000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdbool.h>

#include "i2c.h"
#include "ds1621.h"
#include "uart.h"

void CLK_Init(void);
void TCB0_Init(void);

volatile bool flag = false;

int main(void)
{
	char tbuffer[16];
	uint16_t temp = 0;
	
	CLK_Init();
	TCB0_Init();
	I2C_Init();
	
	USART0_Init(115200);
	
	sprintf(tbuffer,"Hello");
	printf("%s\r\n", tbuffer);
	
	DS1621_START_CONVERT_T();
	
	sei();
	
    while (1) 
    {
		if(flag)
		{
			flag = false;
			sprintf(tbuffer,"%4.1f",((float)(DS1621_READ_TEMPERATURE()>>8) + ((DS1621_READ_TEMPERATURE() & 0x80) ? 0.5 : 0.0)) );
			printf("%s\r\n", tbuffer);
		}
    }
}

void CLK_Init(void)
{
	CCP = CCP_IOREG_gc;
	CLKCTRL.MCLKCTRLB = CLKCTRL_PDIV_4X_gc | CLKCTRL_PEN_bm;
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