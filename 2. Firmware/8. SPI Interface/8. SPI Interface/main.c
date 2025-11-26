#define F_CPU 5000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "spi.h"
#include "uart.h"

void CLK_Init(void);


/* ---------------------------------------
 *  메인
 * --------------------------------------*/
int main(void)
{
	char tBuffer[32] = {0};
	
	CLK_Init();
	USART0_Init(115200);
	
    // SPI 초기화
    SPI_Init();

    // ★ 콜백 등록 — buf를 콜백의 parameter로 전달
    static uint8_t spi_txrx_buf[3] = { 0x40, 0x12, 0xFF };

    // 글로벌 인터럽트 enable
    sei();

	snprintf(tBuffer, sizeof(tBuffer),  "Start SPI");
	printf("%s\r\n",tBuffer);

	
    // 첫 SPI 전송 시작
    SPI_Block_ReadWriteStart(spi_txrx_buf, 3);
	
	// SPI 동작 상태 확인 가능
	_delay_ms(100);
    if (spi_info.handler == SPI_DONE)
    {
	    // spi_txrx_buf 안에는 RX 결과가 있음
	    // ex: spi_txrx_buf[0] = RX0, spi_txrx_buf[1] = RX1, ...

	    // 원하는 처리를 하고
	    // 다시 전송하고 싶으면 이런 식으로:
	    // spi_txrx_buf[0] = next_cmd;
	    // spi_txrx_buf[1] = next_value;
	    // SPI_Block_ReadWriteStart(spi_txrx_buf, 2);
	    snprintf(tBuffer, sizeof(tBuffer),  "handler = %d", spi_info.handler);
	    printf("%s\r\n",tBuffer);
	        
	    spi_info.handler = SPI_IDLE;   // 상태 초기화
    }
    /* ---------------------------------------
     *  메인 루프
     * --------------------------------------*/
    while (1)
    {

    }
    return 0;
}

void CLK_Init(void)
{
	CCP = CCP_IOREG_gc;
	CLKCTRL.MCLKCTRLB = CLKCTRL_PDIV_4X_gc | CLKCTRL_ENABLE_bm;
}