#define F_CPU 5000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include "button.h"

void CLK_Init(void);
void TCB0_Init(void);

#define USER_LED_ON (PORTF.OUTCLR = PIN5_bm)
#define USER_LED_OFF (PORTF.OUTSET = PIN5_bm)

buttonFlag_t buttonFlag;

int main(void)
{
	CLK_Init();
	TCB0_Init();
	BUTTON_LED_GPIO_Init();

	sei();
	while (1)
	{
		if(buttonFlag.OnPress) {}
		if(buttonFlag.Hold) { buttonFlag.Hold = false; USER_LED_ON; }
		if(buttonFlag.DoublePress) { buttonFlag.DoublePress = false; USER_LED_OFF;}
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

/*
 * 스위치(버튼)는 물리적인 접점 특성상 눌림/떼짐 과정에서 미세한 진동이 발생하며,
 * 이로 인해 짧은 시간 동안 불규칙한 ON/OFF 신호가 반복되는 '채터링(Chattering)' 현상이 나타난다.
 * 이러한 노이즈가 그대로 입력으로 처리되면 버튼 신호가 왜곡되어 잘못된 동작을 유발할 수 있다.
 *
 * 하드웨어적으로는 RC Low-Pass Filter 등을 사용하여 채터링을 제거할 수 있으나,
 * 추가 회로 비용이 발생하고 버튼 응답 시간이 느려지는 단점이 있다.
 *
 * 이에 따라 본 코드는 소프트웨어 방식으로 디바운싱(채터링 제거)을 수행한다.
 * 1ms 주기의 Timer Interrupt ISR을 기반으로 하여 5ms마다 buttonISR()을 호출하고,
 * 이 때 입력 상태를 재확인하여 짧은 노이즈를 걸러낸다.
 *
 * goto) 자세한 버튼 상태 관리 구조는 button.h 의 buttonState_t 구조체를 확인하자.
 */

ISR(TCB0_INT_vect)
{
	static uint16_t Cnt1000Hz = 0;
	uint8_t Cnt200Hz = (uint8_t)(Cnt1000Hz % 5);
	Cnt1000Hz++;
	if(Cnt200Hz == 0) buttonISR();
	TCB0.INTFLAGS |= TCB_CAPT_bm;
}