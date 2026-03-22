/*
 * ATmega4809 Internal EEPROM Full Example
 * - EEPROM Write / Read
 * - Struct Load
 * - float printf
 * - 1Hz TCB0 Timer
 */

#define F_CPU 5000000UL
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdio.h>
#include <stdbool.h>
#include <util/delay.h>

#include "uart.h"

// ------------- Struct Definition -------------
typedef struct {
    uint8_t  id;
    uint16_t count;
    float    temperature;
    uint8_t  flags;
} SystemData_t;

// AVR-GCC(AVR-GNU Compiler)가 제공하는
// 특별한 메모리 섹션 지정 키워드(attribute) 이다.
// EEPROM 저장 구조체
SystemData_t EEMEM eeSystemData = { 1, 0, 25.0, 0 };

// ------------- Global -------------
volatile bool flag_1Hz = false;

// ------------- Clock Setup (20MHz → 5MHz) -------------
void CLK_Init(void)
{
    CCP = CCP_IOREG_gc;  
    CLKCTRL.MCLKCTRLB = CLKCTRL_PDIV_4X_gc | CLKCTRL_ENABLE_bm;
}

// ------------- TCB0: 1000Hz Interrupt -------------
void TCB0_Init(void)
{
    TCB0.CCMP = 5000; // 5MHz / 5000 = 1000Hz
    TCB0.CTRLA = TCB_ENABLE_bm;
    TCB0.INTCTRL = TCB_CAPT_bm;
}

// ------------- TCB0 ISR -------------
ISR(TCB0_INT_vect)
{
    static uint16_t tick = 0;

    tick++;
    if (tick >= 1000) { // 1Hz
        tick = 0;
        flag_1Hz = true;
    }

    TCB0.INTFLAGS = TCB_CAPT_bm;
}



// ============================================================
//                       MAIN FUNCTION
// ============================================================
int main(void)
{
    CLK_Init();
    TCB0_Init();
	
    USART0_Init(115200);
	
    sei();

    SystemData_t sys;

    // EEPROM → RAM 구조체 로드
    eeprom_read_block(&sys, &eeSystemData, sizeof(SystemData_t));

    // 첫 로딩 출력
    printf("\r\n=== EEPROM System Data Loaded ===\r\n");
    printf("ID          : %d\r\n", sys.id);
    printf("Count       : %u\r\n", sys.count);
    printf("Temperature : %.1f\r\n", sys.temperature);
    printf("Flags       : %d\r\n", sys.flags);
	
	sys.id = 1;
	sys.count++;
    sys.temperature = 0.1f;
    sys.flags = true;
	
	// 변경 사항 EEPROM 저장
   eeprom_update_block(&sys, &eeSystemData, sizeof(SystemData_t));

    while (1) { }
}
