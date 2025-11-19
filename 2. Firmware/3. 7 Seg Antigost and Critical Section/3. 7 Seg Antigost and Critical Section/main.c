/* 
  * #키워드
  * #7SEGMENT #크리트컬 섹션 #Anti Ghost
  *
  * 3장 7SEG와 크리티컬 섹션을 방지하는 방법
  * 
  * 지난 2장에서 버튼의 여러가지 동작 구현 방법을 알아보았다.
  * 이번 3장에서는 7 SEGMENT를 동작시키는 방법에 대해 알아보자.
  * 
  * 우선 7 SEGMENT를 설정하기 위한 클럭과 인터럽트 설정은 다시 설명하지 않겠다.
  * 기억이 나지 않는다면 1장을 다시 보고 오길 바란다.
  * GPIO에서 버튼 구현을 위해 설정을 조금 바꿨으니 GPIO_Init()을 보고 오자.
  * 
  * callback) GPIO_Init()을 보자.
  * 
  * SEGMENT를 동작시키기 위해서는 우선 Initialization 을 진행해야한다.
  * 이때 하드웨어적으로 다알링톤에 의해 세그먼트의 동작을 유추하는 방법을 알아야한다. (Common Emitter Amp 를 생각해보자)
  * 회로도를 보면 다알링톤에 의해 신호가 뒤집어진다는 사실을 생각하며 다음을 이해해보자.
  *
  * PORTF에 output 을 다알링톤과 연결을 시켜 HIGH 신호를 주게 될 경우 LOW 신호가 LED에 들어가게 된다.
  * 반대편에는 PORTC에 HIGH 신호를 주게 될 경우 PORTF의 LOW와 PORTC의 HIGH에 전압차가 발생하여 LED가 켜지게 된다.
  * 이해가 안된다면 될때까지 이해해봐라.
  * 
  * 이러한 세그먼트의 동작을 암기하고 언제 PORTF에 HIGH/LOW 신호를 줘야하는지 PORTC에 HIGH/LOW 신호를 줘야하는지 정확하게 파악해야한다.
  * 세그먼트를 사용할 경우 디버그 할때 파악할 줄 알아야 한다. 
  *
  * 세그먼트가 모두 안켜질 경우 (Anti Ghost 나 PORT 설정을 잘못했을 가능성 존재)
  * 일부만 안켜질 경우 (Anti Ghost 나 PORT 설정을 잘못했을 가능성 존재)
  * 데이터값이 이상하게 나올 경우 (data value 의 타입이 이상해서 or segBuffer에 넣을 때 다른 값을 넣었을 떄 따라서 break point 를 segDigit에 걸어서 올바른 정보가 들어오는지 확인한다)
  * Reading Zero Kill 이 이상하게 나올 경우 (Reading Zero Kill에서 i값을 잘못 설정했거나 else break;를 안걸어줬을 때) 
  * 이상이 없는거 같은데 LED 하나 동작 안할 경우 (납땜문제나 LED가 과전류에 의해 고장났을 경우가 있다. 보통은 코드문제 때문에 발생하는 것이니 당황하지 말고 코드나 다시 고쳐봐라.)
  * 그 밖에 데이터 처리가 잘못 된 경우 (data 연산 과정의 오류)
  * 
  * 위의 오류들은 SEG를 모두 학습하고 난 후 직접 오류를 발생시켜보면서 어느 부분이 잘못되면 오류가 발생하는지 기록해서 암기해두는 것이 좋다.
  *
  * 본론으로 돌아가 Initialization 을 진행해야한다.
  * 
  * goto) sements.c 파일에 SEG_Init()을 보자.
  */ 
#define F_CPU 5000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include "segments.h"

void CLK_Init(void);
void TCB0_Init(void);

int main(void)
{
	CLK_Init();
	TCB0_Init();
	SEG_Init();
	
	sei();
	
	while (1)
	{
		/*
		 * segSignedDisplay()함수를 만들지 않고 먼저 써넣은 이유는 세그먼트를 언제 부르는지 파악하면 코드를 작성하기 용이하기 때문에 이 순서로 학습을 했다.
		 * 다른 통신 같은 코드를 작성할 때에는 우선 통신 protocol 을 뚫고 interface 와 application 을 작성한다.
		 * 
		 * 세그먼트를 파악하기 위해 제일 먼저
		 * segSignedDisplay(-103);를 사용한다.
		 * 이때 -103을 사용한 이유는 -의 경우와 가운데 0 에서 코드 오류가 발생할 경우의 수가 많기 때문에 -103을 통해 빠르게 디버깅을 할 수 있게 된다.
		 * 예를들어 -1과 3만 나오게 된다면 reading zero kill 이 잘못 설정되어 0을 없애버린 것이고
		 * -의 값이 안나오거나 전혀다른 숫자가 나올 경우 data type이 잘못 되어있거나 (int16_t를 사용해야 하는데 uint16_t를 사용하는 경우) segBuffer[0] == 0x3f가 잘못 되어있을 경우가 많다.
		 * 따라서 -103을 테스트로 사용하는 것이다.
		 * 
		 * segSignedDisplay 를 설정을 했다면 segSignedDisplay() 를 작성해 보자.
		 * 
		 * goto) segSignedDisplay()을 보자.
		 */
		segSignedDisplay(-103);
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

/*
 * 우리 눈은 60Hz 이상의 주파수를 볼 수 없이 때문에 5ms를 기준으로 AntiGhost를 진행한다.
 * 
 * Anti Ghost를 진행하지 않을 경우 Ghost 현상이 보이게 된다.
 * segAntiGhostISR(Cnt1000Hz)를 만들어주자.
 *
 * goto) segments.c파일에 segAntiGhostISR(Cnt1000Hz)
 */
ISR(TCB0_INT_vect)
{
	static uint16_t Cnt1000Hz = 0;
	Cnt1000Hz++;
	uint8_t Cnt200Hz = (uint8_t)(Cnt1000Hz % 5);
	uint16_t Cnt1Hz = (uint16_t)(Cnt1000Hz % 1000);
	
	if(Cnt1Hz == 0);
	if(Cnt200Hz == 0) segAntiGhostISR(Cnt1000Hz);
	
	TCB0.INTFLAGS |= TCB_CAPT_bm;
}
