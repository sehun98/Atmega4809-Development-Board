#define F_CPU 5000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdbool.h>
#include <stdio.h>

#include "uart.h"
#include "spi.h"
#include "mcp23s17.h"
#include "keypad_mcp23s17.h"

void CLK_Init(void);
void TCB0_Init(void);

char KeyMap[16] =
{
    '1','2','3','A',
    '4','5','6','B',
    '7','8','9','C',
    '*','0','#','D'
};

int main(void)
{
    CLK_Init();
    USART0_Init(115200);
    SPI_Init();
    MCP23S17_Init();
    TCB0_Init();

    sei();  // 인터럽트 Enable

    printf("=== KEYPAD TEST START ===\r\n");

    while (1)
    {
        if (gkswFlag)
        {
            gkswFlag = false;

            if (gkswScanCode < 16)
            {
                char key = KeyMap[gkswScanCode];
                printf("KEY PRESSED : %c  (Code=%d)\r\n", key, gkswScanCode);
            }
            else
            {
                printf("KEY RELEASED\r\n");
            }
        }
    }
}

/* ========================== SYSTEM CLOCK ========================== */
void CLK_Init(void)
{
    CCP = CCP_IOREG_gc;
    CLKCTRL.MCLKCTRLB = CLKCTRL_PDIV_4X_gc | CLKCTRL_ENABLE_bm;
}

/* ========================== TIMER (5kHz) ========================== */
/*
 * F_CPU = 5MHz
 * TCB0.CCMP = 5000 → 약 1kHz
 * cnt % 5 → 200Hz keypad 스캔
 */
void TCB0_Init(void)
{
    TCB0.CCMP = 5000;
    TCB0.INTCTRL = TCB_CAPT_bm;
    TCB0.CTRLA = TCB_ENABLE_bm;
}

ISR(TCB0_INT_vect)
{
    static uint16_t cnt = 0;

    if ((cnt++ % 5) == 0)
    {
        ScanKeySwISR();
    }

    TCB0.INTFLAGS = TCB_CAPT_bm;
}