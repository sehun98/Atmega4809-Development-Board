#define F_CPU 5000000UL

#include <avr/io.h>
#include <avr/interrupt.h>

void CLK_Init(void);
void TCB0_Init(void);

// ===============================
//  상태 이름 정의 (enum)
// ===============================
typedef enum {
	STATE_INIT,
	STATE_WORK,
	STATE_FINISH
} state_t;

// ===============================
//  상태 함수 타입
// ===============================
typedef void (*state_callback_t)(void);

// ===============================
//  현재 상태 저장
// ===============================
static state_t current_state = STATE_INIT;
static state_callback_t state_cb = 0;

// ===============================
//  상태 전환 함수
// ===============================
void State_Change(state_callback_t cb, state_t next_state)
{
	state_cb = cb;
	current_state = next_state;
}

// ===================================
//  상태 동작 함수들
// ===================================

void State_Init(void)
{
	// 여기서 초기화 작업 수행
	// 예: LED 꺼짐, 카운터 초기화 등

	// 다음 상태로 전환
	State_Change(State_Work, STATE_WORK);
}

void State_Work(void)
{
	// 반복 작업
	// 예: 센서 읽기, 카운터 증가 등

	// 예: 어떤 조건에서 끝내기
	if (/* 작업 끝남 조건 */ 0) {
		State_Change(State_Finish, STATE_FINISH);
		return;
	}

	// 아니면 계속 Work 상태 유지
	// (State_Change 호출 안 하면 상태 유지)
}

void State_Finish(void)
{
	// 종료 작업 수행
	// 예: LED 켬, flag 세팅 등

	// FSM 종료: state_cb를 NULL로 만든다!
	state_cb = 0;
}


// ===================================
//  메인
// ===================================
int main(void)
{
	CLK_Init();
	TCB0_Init();
	sei();

	// 초기 상태 설정
	State_Change(State_Init, STATE_INIT);

	while (1)
	{
		// FSM은 ISR에서 자동 동작
	}
}


// ===================================
//  타이머 ISR (1ms)
// ===================================
ISR(TCB0_INT_vect)
{
	static uint16_t Cnt1000Hz = 0;
	uint16_t Cnt1Hz = (uint16_t)(Cnt1000Hz % 1000);
	Cnt1000Hz++;
	
	if(Cnt1Hz == 0 && state_cb != 0) state_cb();

	TCB0.INTFLAGS |= TCB_CAPT_bm;
}

// ===================================
//  클럭 / 타이머
// ===================================
void CLK_Init(void)
{
	CCP = CCP_IOREG_gc;
	CLKCTRL.MCLKCTRLB = CLKCTRL_PDIV_4X_gc | CLKCTRL_ENABLE_bm;
}

void TCB0_Init(void)
{
	TCB0.CCMP = 5000;
	TCB0.CTRLB = TCB_CNTMODE_INT_gc;
	TCB0.CTRLA = TCB_ENABLE_bm;
	TCB0.INTCTRL = TCB_CAPT_bm;
}
