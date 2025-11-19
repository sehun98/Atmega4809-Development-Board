#define F_CPU 5000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include "button.h"

extern buttonFlag_t buttonFlag = {false,false,false};

/*
 * PF5 : LED
 * PF6 : BUTTON
 *
 * PORTF의 6번 핀에 버튼이 존재하기 떄문에 이를 내부 pull up 으로 사용하기 위해 다음과 같이 설정을 진행하였다.
 * Direction 설정은 이해하기 쉬울것이다.
 *
 * PORTF.PIN6CTRL |= PORT_PULLUPEN_bm;
 * 내부에서 pull up 설정을 해줌으로 노이즈에 대비를 한다.
 *
 * callback) return 맨 처음으로 되돌아간다.
 */
void GPIO_Init(void)
{
	PORTF.DIRSET = PIN5_bm;
	PORTF.DIRCLR = PIN6_bm;
	PORTF.OUTSET = PIN5_bm;
	PORTF.PIN6CTRL |= PORT_PULLUPEN_bm;
}

/*
 * [1] 클릭(Click) 이벤트
 * 버튼이 눌렸다가 200ms 이상 유지되지 않고 바로 해제되는 경우,
 * 사용자가 '짧게 한 번 눌렀다'고 판단하여 Click 이벤트를 발생시킨다.
 * 이는 일반적인 UI 기준에서 50~100ms 이하의 짧은 눌림은
 * '의도적인 클릭'으로 해석하기 때문이다.
 *
 * [2] 더블 클릭(DoublePress) 판정
 * 이전 눌림(LastPress)과 현재 눌림(CurrentPress) 사이의 시간이
 * 80ms 미만이면 사용자가 매우 빠르게 연속 두 번 누른 상황으로 판단한다.
 *
 * → 왜 80ms인가?
 *   - 사람 손가락으로 가능한 '정말 빠른 연속 입력'은 50~100ms 사이이다.
 *   - 80ms 미만이면 대부분 UI/입력 시스템에서 Double Click의 1차 조건으로 사용된다.
 *   - 80~200ms 사이는 '일반적인 느린 더블클릭'으로 오해될 수 있으므로
 *     Single Click과의 구분을 위해 짧은 타임윈도우(80ms)를 사용한다.
 *
 * 결과적으로 80ms 이하의 간격은 '실수로 누른 것'이 아니라
 * '의도된 더블 클릭'으로 판단하는 데 가장 적합한 범위이다.
 *
 * [3] 홀드(Hold) 판정
 * 버튼이 눌린 상태가 200ms 이상 지속되면 Hold 이벤트를 발생시킨다.
 *
 * → 왜 200ms인가?
 *   - 사람 손가락으로 150~250ms 이상 지속된 입력은 “길게 누르기(Long Press)”로 인식된다.
 *   - 200ms는 채터링(수 ms)이나 짧은 클릭(수십 ms)을 확실히 제외하면서도
 *     입력 반응성을 유지하는 가장 이상적인 범위이다.
 *   - 스마트폰·UI·임베디드 UX에서 Long Press 기준으로 가장 많이 쓰는 값이 200~500ms이다.
 *
 */
void buttonISR(void)
{
	static buttonState_t buttonState = BUTTON_IDLE;
	static uint16_t buttonPressTimeCount = 0;
	static uint16_t LastPress, CurrentPress = 0;
	static uint16_t clock = 0;

	clock++;

	if(LastPress && LastPress != CurrentPress && (CurrentPress - LastPress) < 80) { buttonFlag.DoublePress = true; }
	if(!buttonFlag.Hold && buttonPressTimeCount >= 200) buttonFlag.Hold = true;
	switch (buttonState)
	{
		case BUTTON_IDLE :
			if(USER_BUTTON_ON) buttonState++;
			break;
		case BUTTON_PRESSING :
			if(USER_BUTTON_ON) { buttonFlag.OnPress = true; buttonState++; CurrentPress = clock; }
			else buttonState--;
			break;
		case BUTTON_PRESSED :
			if(!USER_BUTTON_ON) buttonState++;
			else { buttonPressTimeCount++; LastPress = CurrentPress; }
			break;
		case BUTTON_RELEASE :
			if(!USER_BUTTON_ON) { buttonFlag.OnPress = false; buttonState = BUTTON_IDLE; buttonPressTimeCount = 0; }
			else buttonState--;
			break;
	}
}