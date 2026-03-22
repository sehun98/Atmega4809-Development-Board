#define F_CPU	5000000UL

#include "dht11.h"
#include <stdio.h>

/* -------------------------------------------------------------
 * Global Variables
 * - ISR(TCB0/TCB2)와 main()이 공유하는 플래그 및 버퍼
 * ------------------------------------------------------------- */
volatile bool     DHT11_TriggerFlag = false;                 // 1초마다 측정 요청 플래그
volatile bool     DHT11_MeasureFlag = false;                 // 40bit 데이터 수신 완료 플래그
volatile uint8_t  DHT11_highTime[DHT11_MAX_BITS] = {0};      // Capture된 High Pulse 길이값 저장
volatile uint8_t  BitIndex = 0;                              // 현재 수신 중인 비트 인덱스


/* -------------------------------------------------------------
 * DHT11_Init()
 * - TCB2와 이벤트 시스템(EVSYS), PD0 단자 초기화
 * - 1-Wire 데이터 입력 핀을 TCB2 Capture로 직접 연결
 * ------------------------------------------------------------- */
void DHT11_Init(void)
{
    /*
     * TCB2 Capture Mode 설정
     * - CNTMODE = PW : Pulse-Width Measurement
     *   PD0 입력의 에지가 발생할 때마다 CCMP 레지스터에
     *   '이전 에지~현재 에지까지의 시간 차'가 기록된다.
     *
     * - 클럭 = F_CPU(5MHz)/2 = 2.5MHz → 1tick = 0.4us
     */
    TCB2.CTRLB |= TCB_CNTMODE_PW_gc;
    TCB2.EVCTRL |= TCB_CAPTEI_bm;                      // External Event(에지)를 Capture 입력으로 사용
    TCB2.CTRLA  |= TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm;
	TCB2.INTCTRL = TCB_CAPT_bm;
	
    /*
     * EVSYS 설정
     * PORTD.0(PD0)의 에지를 TCB2의 Capture Event로 직접 연결
     * - CHANNEL3 : PD0의 에지 Generator
     * - USERTCB2 : TCB2의 캡처 입력으로 연결
     */
    EVSYS.CHANNEL3 = EVSYS_GENERATOR_PORT1_PIN0_gc;
    EVSYS.USERTCB2 = EVSYS_CHANNEL_CHANNEL3_gc;

    /*
     * PD0 초기 상태 = HIGH(Output)
     * - DHT11 라인은 내부 풀업 구조이므로 평상시 High가 정상
     * - Start 신호에서만 Output Low로 만든 후 다시 Input으로 전환함
     */
    PORTD.DIRSET = PIN0_bm;    // 출력 모드
    PORTD.OUTSET = PIN0_bm;    // HIGH
}


/* -------------------------------------------------------------
 * DHT11_StartTimer()
 * - TCB0을 1ms 타이머로 설정하여
 *   1초마다 DHT11 센싱 시퀀스를 시작하게 함
 * ------------------------------------------------------------- */
void DHT11_StartTimer(void)
{
    TCB0.CCMP = 5000;              // 1ms 주기 (5MHz 기준)
	TCB0.CTRLA |= TCB_ENABLE_bm;
	TCB0.INTCTRL |= TCB_CAPT_bm;
}


/* -------------------------------------------------------------
 * DHT11_Read()
 * - DHT11_highTime[]에 저장된 40bit Pulse 길이를 디코딩
 * - 5바이트(습도/온도/체크섬)로 변환한다.
 *
 * 순서:
 *   1) highTime[] 를 이용하여 bit → byte 조립
 *   2) 체크섬 검증
 *   3) 정상일 경우 습도/온도 값 반환
 * ------------------------------------------------------------- */
bool DHT11_Read(uint8_t *h, uint8_t *t)
{
    uint8_t index = 0;
    uint8_t data[5] = {0};
    uint8_t tempByte = 0;

    /*
     * 40bit (5byte) 디코딩
     * - tempByte를 왼쪽으로 밀고 (<<1)
     * - HighTime이 Threshold보다 크면 Bit=1, 작으면 Bit=0
	 * DHT11 5-Byte Data Format
	 *
	 * Byte[0] : Humidity Integer Part      (습도 정수부)
	 * Byte[1] : Humidity Decimal Part      (습도 소수부, DHT11은 항상 0)
	 *
	 * Byte[2] : Temperature Integer Part    (온도 정수부)
	 * Byte[3] : Temperature Decimal Part    (온도 소수부, DHT11은 항상 0)
	 *
	 * Byte[4] : Checksum
	 *           → Byte[0] + Byte[1] + Byte[2] + Byte[3]
	 */
    for (uint8_t i = 0 ; i < 5 ; i++)
    {
        tempByte = 0;
        for (uint8_t j = 0 ; j < 8 ; j++)
        {
            tempByte <<= 1;
            tempByte += (DHT11_highTime[index++] > DHT11_THRESHOLD) ? 1 : 0;
        }
        data[i] = tempByte;
    }

    /*
     * 체크섬 검증
     *   data[4] == data[0]+data[1]+data[2]+data[3]
     */
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];

    if (checksum == data[4])
    {
        *h = data[0];     // 습도 (정수부만 사용)
        *t = data[2];     // 온도 (정수부만 사용)
        return true;
    }
    return false;
}


/* -------------------------------------------------------------
 * ISR(TCB0_INT_vect)
 * - 1ms 주기로 실행되는 인터럽트
 *
 * 역할:
 *   1) 1000ms(1초) 경과 시 DHT11 Start 시퀀스 시작
 *   2) PD0을 Output Low로 20ms 유지 → Start Signal 생성
 *   3) 20ms 이후 PD0을 Input으로 전환하고 TCB2 Capture 시작
 * ------------------------------------------------------------- */
ISR(TCB0_INT_vect)
{
    static uint16_t DHT11_SEC = 0;
    static DHT11_State_t state = DHT11_IDLE;
    static uint8_t count20ms;

    /* 1초마다 측정 시작 */
    if (++DHT11_SEC >= 1000 && !DHT11_TriggerFlag)
    {
        DHT11_SEC = 0;
        DHT11_TriggerFlag = true;
        state = DHT11_IDLE;
    }

    if (DHT11_TriggerFlag)
    {
        switch (state)
        {
            case DHT11_IDLE:
                /* PD0 = Output Low, 20ms Start Signal */
                PORTD.DIRSET = PIN0_bm;
                PORTD.OUTCLR = PIN0_bm;

                count20ms = 20;
                BitIndex = 0;
                state = DHT11_START_LOW;
                break;

            case DHT11_START_LOW:
                /* 20ms 경과하면 PD0을 Input으로 전환 */
                if (--count20ms == 0)
                {
                    PORTD.DIRCLR = PIN0_bm;     // 입력 모드
                    PORTD.OUTSET = PIN0_bm;     // Pull-up Enable

                    /* TCB2 Capture 시작 */
                    state = DHT11_MEASURE;
                    TCB2.INTFLAGS = TCB_CAPT_bm;   // 플래그 클리어
                    TCB2.INTCTRL = TCB_CAPT_bm;    // 인터럽트 Enable
                }
                break;

            case DHT11_MEASURE:
                /* TCB2 ISR에서 처리됨 */
                break;
        }
    }

    TCB0.INTFLAGS |= TCB_CAPT_bm;
}


/* -------------------------------------------------------------
 * ISR(TCB2_INT_vect)
 * - PD0의 에지 발생 시 실행
 * - 이전 에지~현재 에지까지의 시간(Capture 값)을 저장
 *
 * 구조:
 *   BitIndex < 2 : DHT11 응답 신호(80us Low/80us High) → 무시
 *   BitIndex >= 2 : 데이터 비트(40개) 저장
 *   BitIndex == 41 : 측정 종료 → 플래그 세팅
 * ------------------------------------------------------------- */
ISR(TCB2_INT_vect)
{
    if (BitIndex >= 2)
    {
        /* High Pulse 주기 저장 (0~39번째 데이터 비트) */
        DHT11_highTime[BitIndex - 2] = (uint8_t)TCB2.CCMP;

        /* 40bit 수신 완료 */
        if (BitIndex >= 41)
        {
            PORTD.DIRSET = PIN0_bm;   // PD0 다시 Output으로 복귀
            TCB2.INTCTRL = ~TCB_CAPT_bm;       // Capture Interrupt Disable
            DHT11_TriggerFlag = false;
            DHT11_MeasureFlag = true; // main에서 읽기 시작
        }
    }

    BitIndex++;
    TCB2.INTFLAGS |= TCB_CAPT_bm;
}

