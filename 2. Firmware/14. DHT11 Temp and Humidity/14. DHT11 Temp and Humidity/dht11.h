#ifndef DHT11_H_
#define DHT11_H_

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>

/*
 * #DHT11 Driver (1-Wire Timing 기반)
 *
 * DHT11 센서는 단일 GPIO(PD0)를 통해 통신하며,
 * MCU가 먼저 Start Signal(20ms Low)을 전송한 후
 * 센서의 응답 및 데이터를 타이밍 기반(Pulse Width)으로 수신한다.
 *
 * 본 드라이버는 다음과 같은 하드웨어 구조를 사용한다.
 *
 * 1) TCB0 (1ms 타이머)
 *    - 1초마다 DHT11 센싱을 트리거
 *    - Start Signal(20ms Low)을 만드는 상태머신 실행
 *
 * 2) TCB2 (Capture 모드, 2.5MHz = 0.4us Resolution)
 *    - PD0의 상승/하강 Edge마다 CCMP에 High 시간 저장
 *    - 총 40개의 High Pulse 길이를 확보하여 0/1 비트 판단
 *
 * 3) Pulse Width 기반 Bit 판별
 *    - DHT11 데이터는 40bit (5 byte)
 *    - 각 비트의 High 유지 시간으로 0/1을 구분한다.
 *
 *      비트 0 → 26~28us (짧은 High)
 *      비트 1 → 약 70us (긴 High)
 *
 *    - 두 값의 중간값 (≈48us)을 기준으로 비교한다.
 *    - TCB2는 0.4us Tick → 48us ≈ 48 / 0.4 = 120 Tick 수준
 *    - 따라서 Threshold 116을 기준으로 0/1 구분
 *
 *      HighTime > 116 → Bit = 1
 *      HighTime ≤ 116 → Bit = 0
 */

#define DHT11_THRESHOLD   122   // CCMP 값 116 Tick ≈ 46.4us → 0/1 Bit 판별 기준
#define DHT11_MAX_BITS    40      // DHT11은 총 40bit 전송 (습도/온도 5byte)

/*
 * #State Machine for DHT11 Control
 * - DHT11_IDLE       : 다음 측정을 기다리는 상태
 * - DHT11_START_LOW  : MCU가 PD0을 20ms 동안 LOW로 유지하는 구간
 * - DHT11_MEASURE    : 센서 응답 및 데이터 비트 수신 단계
 */
typedef enum {
    DHT11_IDLE,
    DHT11_START_LOW,
    DHT11_MEASURE
} DHT11_State_t;

/* ---- 글로벌 변수 (main.c 및 ISR에서 사용) ---- */
extern volatile bool     DHT11_TriggerFlag;                 // 1초마다 센싱 요청 플래그
extern volatile bool     DHT11_MeasureFlag;                 // 데이터 수신 완료 플래그
extern volatile uint8_t  DHT11_highTime[DHT11_MAX_BITS];    // 각 비트 High 시간 저장 버퍼
extern volatile uint8_t  BitIndex;                          // 현재 수신 중인 Bit Index

/* ---- 함수 선언 ---- */

void DHT11_Init(void);            // TCB2 + EVSYS + PD0 초기화
void DHT11_StartTimer(void);      // TCB0 (1ms Timer) 설정

// 40bit 데이터를 5byte로 조립 후 습도/온도 값을 반환
bool DHT11_Read(uint8_t *humidity, uint8_t *temperature);

#endif /* DHT11_H_ */
