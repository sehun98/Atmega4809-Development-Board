#define F_CPU 5000000UL   // 5 MHz

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdbool.h>
#include <stdio.h>

#include "spi.h"

/* 전역 SPI 상태 */
spi_info_t spi_info;
uint8_t    spi_buff[32];   // 필요시 사용하는 공용 버퍼(예제용)

/* -------------------- 기본 콜백들 -------------------- */

spi_operation_t SPI_Stop_CB(void *p)
{
    (void)p;
    return SPI_STOP;
}

spi_operation_t SPI_ReadStop_CB(void *p)
{
    (void)p;
    return SPI_READ_STOP;
}

/* -------------------- 콜백 설정 -------------------- */

void SPI_SetDataXferCompleteCallBack(spi_callback_t cb, void *p)
{
    if (cb)
    {
        spi_info.callbackDataXferComplete = cb;
        spi_info.callbackParameter        = p;
    }
    else
    {
        spi_info.callbackDataXferComplete = SPI_Stop_CB;
        spi_info.callbackParameter        = NULL;
    }
}

/* -------------------- 초기화 -------------------- */

void SPI_Init(void)
{
    /* 구조체 초기화 */
    spi_info.data_ptr    = NULL;
    spi_info.data_length = 0;
    spi_info.handler     = SPI_FREE;
    SPI_SetDataXferCompleteCallBack(SPI_Stop_CB, NULL);   // 기본 콜백 설정

    // MOSI(PA4), SCK(PA6), SS(PA7) → 출력
    // MISO(PA5) → 입력
    PORTA.DIRSET = PIN4_bm | PIN6_bm | PIN7_bm;
    PORTA.DIRCLR = PIN5_bm;

    PORTA.OUTSET = PIN7_bm;   // SS HIGH (inactive)

    // SPI CTRLA 설정
    // - Master Mode
    // - SPI Enable
    // - Prescaler (DIV4 => F_SPI = F_CPU/4 = 1.25MHz)
    SPI0.CTRLA = SPI_MASTER_bm | SPI_ENABLE_bm | SPI_PRESC_DIV4_gc;

    // SPI CTRLB 설정
    // - SPI Mode 0 (CPOL=0, CPHA=0)
    SPI0.CTRLB = SPI_MODE_0_gc;

    // 인터럽트 활성화
    SPI0.INTCTRL = SPI_IE_bm;

    spi_info.handler = SPI_IDLE;
}

/* -------------------- 전송 시작 -------------------- */

void SPI_Block_ReadWriteStart(uint8_t *buffer, uint8_t length)
{
    if ((buffer == NULL) || (length == 0))
        return;

    /* 이미 BUSY면 무시 (상황에 따라 assert 넣어도 됨) */
    if (spi_info.handler == SPI_BUSY)
        return;

    spi_info.data_ptr    = buffer;
    spi_info.data_length = length;
    spi_info.handler     = SPI_BUSY;

    // 첫 바이트 전송 시작
    SS_LOW;
    SPI0.DATA = *spi_info.data_ptr;
}

/* -------------------- 인터럽트 서비스 루틴 -------------------- */

ISR(SPI0_INT_vect)
{
    uint8_t rx_data = SPI0.DATA;   // 수신 데이터 먼저 읽음

    // 혹시라도 length가 0이면 이상 상황 → 그냥 플래그만 클리어
    if (spi_info.data_length == 0)
    {
        SPI0.INTFLAGS = SPI_IF_bm;
        return;
    }

    // 현재 위치에 수신 데이터 저장 (in-place)
    *spi_info.data_ptr = rx_data;

    // 다음 위치로 이동
    spi_info.data_ptr++;
    spi_info.data_length--;

    if (spi_info.data_length > 0)
    {
        // 아직 남은 바이트가 있으면 다음 바이트 전송
        SPI0.DATA = *spi_info.data_ptr;
    }
    else
    {
        // 모든 바이트 전송 완료
        SS_HIGH;

        spi_operation_t op = SPI_STOP;

        if (spi_info.callbackDataXferComplete)
            op = spi_info.callbackDataXferComplete(spi_info.callbackParameter);

        switch (op)
        {
            case SPI_RESTART_BLOCK:
            case SPI_READ_WRITE:
                if ((spi_info.data_ptr != NULL) && (spi_info.data_length > 0))
                {
                    spi_info.handler = SPI_BUSY;
                    SS_LOW;
                    SPI0.DATA = *spi_info.data_ptr;
                }
                else
                {
                    spi_info.handler = SPI_DONE;
                }
                break;

            case SPI_READ_STOP:
                /* 읽기 종료 후 STOP */
                spi_info.handler = SPI_DONE;
                break;

            case SPI_STOP:
            default:
                spi_info.handler = SPI_DONE;
                break;
        }
    }

    // 인터럽트 플래그 클리어
    SPI0.INTFLAGS = SPI_IF_bm;
}
