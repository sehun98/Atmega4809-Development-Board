#ifndef SPI_H_
#define SPI_H_

#include <stdbool.h>
#include <stdint.h>
#include <avr/io.h>

#define SS_HIGH  (PORTA.OUTSET = PIN7_bm)
#define SS_LOW   (PORTA.OUTCLR = PIN7_bm)

/* SPI 상태 */
typedef enum
{
    SPI_FREE,
    SPI_IDLE,
    SPI_BUSY,
    SPI_DONE
} spi_handler_t;

/* 전송이 끝난 뒤, 다음에 무엇을 할지 콜백이 알려줌 */
typedef enum
{
    SPI_READ_WRITE,     // 같은 버퍼/설정으로 계속 전송
    SPI_RESTART_BLOCK,  // (콜백에서) 새 버퍼/길이 세팅 후 다시 시작
    SPI_READ_STOP,      // 읽기 종료 후 STOP
    SPI_STOP            // 그냥 STOP
} spi_operation_t;

/* 콜백 타입 */
typedef spi_operation_t (*spi_callback_t)(void *p);

/*
 * SPI 정보 구조체
 * - 이 드라이버는 "in-place 버퍼" 방식을 사용함
 *   => buffer에 들어 있던 값이 TX로 나가고,
 *      동시에 같은 위치에 RX 값이 덮어써진다.
 */
typedef struct
{
    uint8_t  *data_ptr;       // 현재 TX/RX 위치를 가리키는 포인터
    uint8_t   data_length;    // 남은 바이트 수

    volatile spi_handler_t handler;

    spi_callback_t callbackDataXferComplete;
    void          *callbackParameter;
} spi_info_t;

/* 전역 상태 (필요하면 외부에서 handler만 읽어도 됨) */
extern spi_info_t spi_info;

/* API */
void SPI_Init(void);
void SPI_SetDataXferCompleteCallBack(spi_callback_t cb, void *p);

/*
 * SPI_Block_ReadWriteStart
 * - buffer : 전송 + 수신에 사용하는 in-place 버퍼
 * - length : 전송/수신할 바이트 수
 *
 * 사용 예)
 * uint8_t buf[3] = { cmd, addr, dummy };
 * SPI_Block_ReadWriteStart(buf, 3);
 * => 전송이 끝나면 buf[0..2] 에는 수신 값이 덮어써짐
 */
void SPI_Block_ReadWriteStart(uint8_t *buffer, uint8_t length);

/* 기본 콜백들(원하면 외부에서 사용해도 됨) */
spi_operation_t SPI_Stop_CB(void *p);
spi_operation_t SPI_ReadStop_CB(void *p);

#endif /* SPI_H_ */
