#ifndef UART_H_
#define UART_H_

#define USART0_BAUD_RATE(BAUD_RATE)		((64.0 * F_CPU / (16.0 * BAUD_RATE)) + 0.5)
#define USART0_BUFFER_SIZE				32	// 2^n  <= 256
#define USART0_BUFFER_MASK				(USART0_BUFFER_SIZE - 1)

typedef struct {
	uint8_t RingBuffer[USART0_BUFFER_SIZE];
	uint8_t	HeadIndex, TailIndex;
	uint8_t NoElement;
} RingBuffer_t;

void USART0_Init( uint32_t baud );
uint8_t USART0_GetChar( void );
uint8_t USART0_PutChar( uint8_t dat );
bool USART0_CheckRxData( void );
int	StdIO_Get( FILE *stream );
int StdIO_Put( char d, FILE *stream );

#endif /* UART_H_ */