#ifndef PCF8563_H_
#define PCF8563_H_

/*
 * #PCF8563 #RTC #I2C
 *
 * PCF8563는 NXP 사의 Real-Time Clock(RTC) IC 이다.
 * 초/분/시/일/요일/월/년 정보를 I2C를 통해 읽고 쓸 수 있으며,
 * 내부 레지스터는 BCD(Binary-Coded Decimal) 형식으로 저장된다.
 *
 * 아래는 PCF8563의 기본 Slave Address 및 주요 레지스터 주소이다.
 */

#define PCF8563_ADDR            0x51    // 7bit I2C Slave Address (1010 0001b)

/*
 * 레지스터 주소 정의
 * Control_Status_1 (0x00) : STOP/TEST 비트 포함, RTC 동작 제어
 * Seconds (0x02)           : 초(second) 저장 레지스터
 * 기타 레지스터는 PCF8563_readTimeDate() 함수에서 내부적으로 접근한다.
 */
#define PCF8563_ControlStatus1  0x00
#define PCF8563_Seconds         0x02

/*
 * #CLOCK_t #구조체
 *
 * PCF8563의 시간/날짜 레지스터 구조와 동일한 형태로 구성한 사용자 구조체이다.
 * PCF8563_readTimeDate()를 호출하면 RTC 값이 이 구조체에 저장되고,
 * 날짜/시간 문자열 변환 함수들은 이 구조체 값을 기반으로 동작한다.
 *
 * 각 필드:
 *  seconds  : 초 (0~59)
 *  minutes  : 분 (0~59)
 *  hours    : 시 (0~23)
 *  days     : 일 (1~31)
 *  weekdays : 요일 (0=일요일 ~ 6=토요일)
 *  months   : 월 (1~12)
 *  years    : 연도 (00~99 → 2000~2099로 매핑)
 */
typedef struct PCF8563_REG {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t days;
    uint8_t weekdays;
    uint8_t months;
    uint8_t years;
} CLOCK_t;

/*
 * #PCF8563_wrieTimeDate
 *
 * I2C를 통해 PCF8563의 시/분/초/년/월/일/요일 레지스터를 설정한다.
 * 인자는 BCD 변환 전의 10진수 값이며 함수 내부에서 BCD로 변환하게 된다.
 *
 * 매개변수:
 *   hr  : 시
 *   min : 분
 *   sec : 초
 *   yr  : 연도(0~99 → 2000~2099)
 *   mon : 월
 *   day : 일
 *   dow : 요일 (0=일요일)
 *
 * callback) 구조체 기반 함수 버전으로 리팩토링 가능.
 */
void PCF8563_wrieTimeDate(uint8_t hr, uint8_t min, uint8_t sec,
                          uint8_t yr, uint8_t mon, uint8_t day, uint8_t dow);

/*
 * #PCF8563_readMinSec
 *
 * RTC에서 "분/초"만 빠르게 읽어오기 위한 함수.
 * 반환값 구조: 0xMMSS (상위 바이트=분, 하위 바이트=초)
 *
 * 예) 0x0A14 → 10분 20초
 */
uint16_t PCF8563_readMinSec(void);

/*
 * #PCF8563_readDateStringKR
 *
 * PCF8563_readTimeDate()로 RTC 값을 읽은 후 날짜를 한국식(YYYY-MM-DD) 문자열로 변환한다.
 *
 * 예) "2025-02-11"
 */
void PCF8563_readDateStringKR(char * buff);

/*
 * #PCF8563_readDateStringUS
 *
 * 날짜를 미국식(MM-DD-YYYY) 문자열로 변환한다.
 *
 * 예) "02-11-2025"
 */
void PCF8563_readDateStringUS(char * buff);

/*
 * #PCF8563_readTimeString
 *
 * 시/분 문자열을 생성하는 함수.
 * ap = true  → 12시간제(AM/PM)
 * ap = false → 24시간제
 *
 * 예)
 *   ap=false → "23:41"
 *   ap=true  → "11:41 PM"
 */
void PCF8563_readTimeString(char buff[], bool ap);

/*
 * #PCF8563_readDayOfWeek
 *
 * 요일 번호(0~6)를 문자열(한국/영문)로 변환한다.
 * b=true  → 한국식 ("일", "월", "화"…)
 * b=false → 영어식 ("SUN", "MON"…)
 */
void PCF8563_readDayOfWeek(char* buff, bool b);

#endif /* PCF8563_H_ */
