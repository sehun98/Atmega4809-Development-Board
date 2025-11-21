#include "rotary.h"

volatile	bool	rotDirectionFlag = false;
volatile	bool	rotSwitchFlag = false;
volatile	int16_t	buttonCount = 0;

/*
 * #Initialization
 * 로터리 엔코더는 모두 입력이므로 DIRCLR을 사용하여 입력으로 설정한다.
 * RotSW_S1, RotSW_S2 → Quadrature 입력
 * RotSW_Key → Switch 입력
 *
 * PORTE.DIRCLR은 해당 비트를 0으로 만들어 입력모드로 설정한다.
 *
 * goto) GetRotSwitch()을 보자.
 */
void ROT_Init(void)
{
	PORTE.DIRCLR = RotSW_S1 | RotSW_S2 | RotSW_Key;
}

/*
 * #QuadratureDigitalRead
 *
 * 엔코더의 S1, S2는 PORTE.IN으로 읽어올 수 있다.
 * 하지만 회전 방향 검출을 위해서는 단순히 값만 읽는 것이 아니라
 * "2개의 비트를 하나의 2bit 값(00, 01, 10, 11)"으로 묶어서 사용해야 한다.
 *
 * (~PORTE.IN)을 사용하는 이유:
 * 엔코더는 풀업(Pull-up)을 사용하므로 눌리지 않은 상태는 High(1)이다.
 * 따라서 눌렸을 때 Low(0)가 되므로 bit inversion 후 계산하는 것이 직관적이다.
 *
 * (RotSW_S1 | RotSW_S2) >> 2:
 * 하드웨어 연결에 따라 S1,S2가 P2,P3 등 상위 비트에 있을 수 있기 때문에
 * 회전 FSM이 사용하기 좋게 2bit 범위로 정렬해준다.
 *
 * goto) GetRotDirectionISR()에서 사용된다.
 */
uint8_t GetRotSwitch(void)
{
	return (~PORTE.IN & ( RotSW_S1 | RotSW_S2 )) >> 2;
}

/*
 * #SwitchStateMachine
 * #Debounce #PushSwitch
 *
 * 엔코더의 Push Switch는 Bounce(채터링)가 존재하기 때문에
 * 단순히 눌림/떼짐을 바로 판단하면 오동작이 발생한다.
 *
 * 따라서 FSM(Finite State Machine)을 사용하여 Press → Pressing → Release의
 * 명확한 상태 전이를 감시한다.
 *
 * ROT_IDLE     : 기본 상태
 * ROT_PRESSING : 눌림 감지(디바운스 구간)
 * ROT_PRESSED  : 완전히 눌림
 * ROT_RELEASE  : 릴리즈 처리
 *
 * rotSwitchFlag = true; 를 통해 main() loop에서 처리하도록 플래그만 전달한다.
 *
 * callback) main()의 Super Loop에서 rotSwitchFlag 처리 부분을 보자.
 */
void RotSwitchISR(void)
{
	static RotSwState_t RotSwState = ROT_IDLE;
	
	switch ( RotSwState ) {
		case ROT_IDLE :
			if ( !ROT_BUTTON )  RotSwState = ROT_PRESSING;
			break;
		case ROT_PRESSING :
			if ( ROT_BUTTON ) RotSwState = ROT_IDLE;
			else {
				RotSwState = ROT_PRESSED;
				rotSwitchFlag = true;
			}
			break;
		case ROT_PRESSED :
			if ( ROT_BUTTON ) RotSwState = ROT_RELEASE;
			break;
		case ROT_RELEASE :
			RotSwState = ( ROT_BUTTON )? ROT_IDLE : ROT_PRESSED;
			break;
	}
}


/*
 * #QuadratureDecoding
 * #4StepDecoding
 * #StateMachine
 *
 * Rotary Encoder는 다음과 같은 4개의 상태 조합이 존재한다.
 *
 *      S1  S2
 * S00 : 0   0
 * S01 : 0   1
 * S10 : 1   0
 * S11 : 1   1
 *
 * 이 4개의 조합은 회전 방향에 따라 다음과 같은 순서로 변화한다.
 *
 *  CW (시계 방향)     : S00 → S10 → S11 → S01 → S00
 *  CCW(반시계 방향)  : S00 → S01 → S11 → S10 → S00
 *
 * RotCount는 각 상태에서 +1 또는 -1 증가/감소하며,
 * S00 상태로 돌아왔을 때 RotCount가 +4 or -4가 되었는지를 보고
 * 한 스텝의 회전이 완성되었음을 알린다.
 *
 * buttonCount는 0~9 범위를 유지하며 rotDirectionFlag = true 를 통해
 * main() loop에서 이 값을 출력하거나 동작하도록 한다.
 * 앞서 GetRotSwitch()로 읽어온 값이 여기서 사용된다.
 *
 * goto) main()의 Super Loop에서 rotSwitchFlag 처리 부분을 보자.
 */
void RotDirectionISR(void)
{
	static RotState_t RotState = S00;
	static int8_t	RotCount = 0;
	
	switch ( RotState ) {
		case S00 :
			if ( RotCount ) {
				if ( RotCount == 4 )	{ buttonCount--;  if ( buttonCount < 0 ) buttonCount = 9;  rotDirectionFlag = true; }
				if ( RotCount == -4 )	{ buttonCount++;  if ( buttonCount > 9 ) buttonCount = 0;  rotDirectionFlag = true; }
				RotCount = 0;
			}
			if ( GetRotSwitch() == 0b10 ) { RotState = S10;  RotCount++; }
			if ( GetRotSwitch() == 0b01 ) { RotState = S01;  RotCount--; }
			break;
		case S01 :
			if ( GetRotSwitch() == 0b00 ) { RotState = S00;  RotCount++; }
			if ( GetRotSwitch() == 0b11 ) { RotState = S11;  RotCount--; }
			break;
		case S10 :
			if ( GetRotSwitch() == 0b11 ) { RotState = S11;  RotCount++; }
			if ( GetRotSwitch() == 0b00 ) { RotState = S00;  RotCount--; }
			break;
		case S11 :
			if ( GetRotSwitch() == 0b01 ) { RotState = S01;  RotCount++; }
			if ( GetRotSwitch() == 0b10 ) { RotState = S10;  RotCount--; }
			break;
		default:
			RotState = S00;
			break;
	}
}