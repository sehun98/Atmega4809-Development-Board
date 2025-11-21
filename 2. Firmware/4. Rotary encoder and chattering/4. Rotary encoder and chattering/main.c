/*
 * #RotaryEncoder
 * #Quadrature #StateMachine #SwitchInterrupt
 *
 * 본 장에서는 로터리 엔코더의 두 가지 핵심 기능을 구현한다.
 *
 * 1) 회전(Quadrature Signal - S1, S2)
 * 2) 스위치 클릭(Push Switch)
 *
 * 로터리 엔코더는 단순한 입력장치 같지만 내부적으로
 * 2개의 위상(Phase A, Phase B)이 90도 위상차를 두고 발생한다.
 * 이 2개의 신호 조합에 따라 CW(시계방향), CCW(반시계) 방향을 판단할 수 있다.
 *
 * 또한 엔코더 중앙의 Push Switch는 별도의 핀으로 들어오므로
 * FSM(Finite State Machine)을 사용해 클릭/홀드/릴리즈 등을 판단한다.
 *
 * goto) rotary.c파일에 ROT_Init()을 보자.
 */

#define F_CPU	5000000UL

#include <avr/io.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <stdbool.h>

#include "rotary.h"

void CLK_Init(void);
void TCB0_Init(void);

int main(void)
{
	CLK_Init();
	TCB0_Init();
	
	SEG_Init();
	ROT_Init();
	
	sei();
	
	while (1)
	{
		if ( rotDirectionFlag ) {
			rotDirectionFlag = false;
			segSignedDisplay(buttonCount);
		}
		if ( rotSwitchFlag ) {
			rotSwitchFlag = false;
		}
	}
}
void CLK_Init(void)
{
	CCP = CCP_IOREG_gc;
	CLKCTRL.MCLKCTRLB = CLKCTRL_PDIV_4X_gc | CLKCTRL_ENABLE_bm;
}

// TCB0 Initialization for F_CPU 5000000 / 5000 = 1000 Hz periodic interrupt
void TCB0_Init(void)
{
	TCB0.CCMP = 5000;
	TCB0.CTRLA |= TCB_ENABLE_bm;
	TCB0.INTCTRL |= TCB_CAPT_bm;
}

// 1kHz(1ms)
ISR( TCB0_INT_vect ) {
	static uint16_t Cnt1000Hz = 0;
	Cnt1000Hz++;
	uint8_t Cnt200Hz = (uint8_t)(Cnt1000Hz % 5);
	
	if(Cnt200Hz == 0) segAntiGhostISR(Cnt1000Hz);
	if(Cnt200Hz == 1) RotSwitchISR();

	RotDirectionISR();
	
	TCB0.INTFLAGS |= TCB_CAPT_bm;
}