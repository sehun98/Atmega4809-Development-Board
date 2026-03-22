#define F_CPU	5000000UL		// Max System Clock Frequency at 4.5V ~ 5.5V VDD

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <stdio.h>

#include "uart.h"

RingBuffer_t RxBuffer, TxBuffer;

static  FILE OUTPUT_device = FDEV_SETUP_STREAM( StdIO_Put, NULL, _FDEV_SETUP_WRITE);
static  FILE INPPUT_device = FDEV_SETUP_STREAM( NULL, StdIO_Get, _FDEV_SETUP_READ);

void USART0_Init( uint32_t baud ) {
	USART0.BAUD = (uint16_t)USART0_BAUD_RATE(baud);
	PORTA.DIRSET = PIN0_bm;
	PORTA.OUTSET = PIN0_bm;		// TxD Outmode, TxD = '1'
	USART0.CTRLB |= USART_RXEN_bm + USART_TXEN_bm;
	USART0.CTRLA |= USART_RXCIE_bm;
	
	RxBuffer.HeadIndex = RxBuffer.TailIndex = 0;
	RxBuffer.NoElement = 0;
	TxBuffer.HeadIndex = TxBuffer.TailIndex = 0;
	TxBuffer.NoElement = 0;
	
	while ( USART0.STATUS & USART_RXCIF_bm ) USART0.RXDATAL;
	
	stdout = &OUTPUT_device;
	stdin  = &INPPUT_device;
}

ISR( USART0_RXC_vect ) {
	uint8_t	rxDat;
	
	rxDat = USART0.RXDATAL;
	if ( RxBuffer.NoElement < USART0_BUFFER_SIZE ) {
		RxBuffer.RingBuffer[RxBuffer.HeadIndex++] = rxDat;
		RxBuffer.HeadIndex &= USART0_BUFFER_MASK;
		RxBuffer.NoElement++;
	} else
	;
}

bool USART0_CheckRxData( void ) {
	return RxBuffer.NoElement > 0;
}

uint8_t USART0_GetChar( void ) {
	uint8_t rxDat;
	
	while ( RxBuffer.NoElement == 0 ) ;
	rxDat = RxBuffer.RingBuffer[RxBuffer.TailIndex++];
	RxBuffer.TailIndex &= USART0_BUFFER_MASK;
	cli();  RxBuffer.NoElement--;  sei();
	return rxDat;
}

uint8_t USART0_PutChar( uint8_t dat ) {
	while (TxBuffer.NoElement >= USART0_BUFFER_SIZE) ;
	TxBuffer.RingBuffer[TxBuffer.HeadIndex++] = dat;
	TxBuffer.HeadIndex &= USART0_BUFFER_MASK;
	cli();  TxBuffer.NoElement++;  sei();
	
	USART0.CTRLA |= USART_DREIE_bm;			// local int enable
	
	return dat;
}

ISR( USART0_DRE_vect ) {
	if ( TxBuffer.NoElement > 0 ) {
		USART0.TXDATAL = TxBuffer.RingBuffer[TxBuffer.TailIndex++];
		TxBuffer.TailIndex &= USART0_BUFFER_MASK;
		TxBuffer.NoElement--;
		} else {
		USART0.CTRLA &= ~USART_DREIE_bm;	// local int disable
	}
}

// std I/O
int	StdIO_Get( FILE *stream ) {
	return (int)USART0_GetChar();
}

int StdIO_Put( char d, FILE *stream ) {
	USART0_PutChar( (uint8_t)d );
	return 0;
}