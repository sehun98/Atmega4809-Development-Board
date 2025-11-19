#ifndef BUTTON_H_
#define BUTTON_H_

#define USER_BUTTON_ON !(PORTF.IN & PIN6_bm)

/*
 * 버튼 입력은 물리적 접점 특성상 채터링(Chattering)이 발생하기 때문에,
 * 단순히 ON/OFF만 읽으면 잘못된 동작을 유발할 수 있다.
 * 따라서 FSM(Finite State Machine)을 적용하여,
 * “의도적인 눌림/해제인지”를 단계별로 재확인하며 소프트웨어 디바운싱을 수행한다.
 *
 * FSM은 총 4개의 상태로 구성된다.
 *
 * (1) BUTTON_IDLE
 *     - 정상 대기 상태
 *     - 눌림이 감지되면 → BUTTON_PRESSING 으로 전이
 *
 * (2) BUTTON_PRESSING
 *     - 눌림 디바운싱 단계
 *     - 눌림이 유지되면 → BUTTON_PRESSED (OnPress 이벤트 발생)
 *     - 눌림이 해제되면 → BUTTON_IDLE (채터링으로 판단하여 회귀)
 *
 * (3) BUTTON_PRESSED
 *     - 안정적으로 눌린 상태
 *     - 눌림 유지 시 → 누른 시간 카운트 증가 (Hold 감지)
 *     - 눌림 해제 시 → BUTTON_RELEASE 로 전이
 *
 * (4) BUTTON_RELEASE
 *     - 해제 디바운싱 단계
 *     - 눌림이 다시 감지되면 → BUTTON_PRESSED (채터링으로 판단하여 회귀)
 *     - 눌림 해제가 유지되면 → BUTTON_IDLE (OnPress 종료, 누른 시간 초기화)
 *
 * goto) 구체적인 상태 처리 로직은 button.c 의 buttonISR()을 참고하자.
 */

typedef enum { BUTTON_IDLE, BUTTON_PRESSING, BUTTON_PRESSED, BUTTON_RELEASE }buttonState_t;
typedef struct {
	volatile bool OnPress;
	volatile bool DoublePress;
	volatile bool Hold;
}buttonFlag_t;

void BUTTON_LED_GPIO_Init(void);
void buttonISR(void);

#endif /* BUTTON_H_ */