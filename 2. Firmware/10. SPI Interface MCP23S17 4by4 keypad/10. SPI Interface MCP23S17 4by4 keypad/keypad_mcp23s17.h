#ifndef KEYPAD_MCP23S17_H_
#define KEYPAD_MCP23S17_H_

#include <stdbool.h>
#include <stdint.h>

extern volatile uint8_t KeySwCol;      // 현재 컬럼 상태 (하위 4비트)
extern volatile uint8_t gkswScanCode;  // 0~15 키 번호, 또는 0xFF
extern volatile bool    gkswFlag;      // 새 키 이벤트 발생 플래그

void ScanKeySwISR(void);   // 타이머 ISR에서 호출
#endif /* KEYPAD_MCP23S17_H_ */
