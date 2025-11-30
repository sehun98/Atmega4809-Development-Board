#ifndef DS1621_H_
#define DS1621_H_

#define DS1621_SLAVE_ADDRESS 0x48

#define START_CONVERT_T 0xEE // 온도 변환 시작 명령
#define READ_TEMPERATURE 0xAA // 온도 값을 읽어오는 명령

#define READ_COUNTER 0xA8 // 현재 1°C 범위에서 얼마나 남았는지 알려주는 카운터 값
#define READ_SLOPE 0xA9 // Slope 보정값

#define STOP_CONVERT_T 0x22 // 온도 변환 중단 명령

// Tout Pin을 통해 온도 인터럽트가 발생한다.
#define ACCESS_TH 0xA1 // 온도 경계 레지스터 High Limit
#define ACCESS_TL 0xA2 // 온도 경계 레지스터 Low Limit

/* 
 * 변경할 일이 없을 것 같아서 함수 구현은 안했다.
 * Bit 2 : High → 온도 1회 변환(One-Shot)
 * Bit 1 : Low  → Tout 핀 High Idle 상태
 * Bit 0 : Low  → Comparator 모드(TH/TL 벗어나면 즉시 Active)
 */
#define ACCESS_CONFIG 0xAC // 동작 모드 설정

void DS1621_START_CONVERT_T(void);
uint16_t DS1621_READ_TEMPERATURE(void);

void DS1621_STOP_CONVERT_T(void);

void DS1621_SetHighLimit(uint8_t);
void DS1621_SetLowLimit(uint8_t);

#endif /* DS1621_H_ */