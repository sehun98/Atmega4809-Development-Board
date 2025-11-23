/*
 * #InternalTemperatureSensor
 * #ADC #Calibration #SIGROW
 *
 * 이번 장에서는 ATmega4809 내부에 존재하는 "온도 센서(Internal Temperature Sensor)"를
 * ADC0를 통해 읽는 과정을 살펴보겠다.
 *
 * 내부 온도 센서는 별도의 외부 하드웨어 없이 칩 자체에서 온도를 읽을 수 있기 때문에,
 * 시스템 보호(과열 감지)나 보정(Calibration)에 사용된다.
 *
 * 하지만 이 센서는 일반적인 Analog pin과는 다르게
 * ① SIGROW(Factory Calibration Row)에 저장된 보정값(offset, gain)을 반드시 읽어와야 하고  
 * ② 내부 Reference 전압(1.1V INTREF)을 사용해야 하며  
 * ③ MUXPOS를 TEMPSENSE로 설정해야 정상적인 온도 계산이 가능하다.
 *
 * 이번 예제에서는 온도 값을 Kelvin(K) 기준으로 계산하여 temperature_in_K에 저장한다.
 *
 * goto) internal_temp_adc.c에 ADC0_Init()을 보자.
 */
#define F_CPU 5000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include "segments.h"
#include "internal_temp_adc.h"

void CLK_Init(void);
void TCB0_Init(void);

int main(void)
{
	CLK_Init();
	TCB0_Init();
	ADC0_Init();
	SEG_Init();
	
	sei();
	
    while (1) 
    {
		segUnsignedData(temperature_in_K);
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
	
	EVSYS.CHANNEL0 = EVSYS_GENERATOR_TCB0_CAPT_gc;
	EVSYS.USERADC0 = EVSYS_CHANNEL_CHANNEL0_gc;
}

ISR(TCB0_INT_vect)
{
	static uint16_t Cnt1000Hz = 0;
	uint8_t Cnt200Hz = (uint8_t)(Cnt1000Hz % 5);
	Cnt1000Hz++;
	
	if(Cnt200Hz == 0) segAntiGhostISR(Cnt1000Hz);
	//if(Cnt200Hz == 0) ADC0.COMMAND |= ADC_STCONV_bm;
	
	TCB0.INTFLAGS |= TCB_CAPT_bm;
}



