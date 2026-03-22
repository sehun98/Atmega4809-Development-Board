/*
 * 16 Ultrasonic Sensor.c
 *
 * Created: 2025-11-20 오전 9:21:18
 * Author : tpsk4
 */ 

#include <avr/io.h>


int main(void)
{
    /* Replace with your application code */
    while (1) 
    {
    }
}#define PF5_ALT
// ECHO : Pulse Width Measurement for HC-SR04
TCB2.CTRLB |= TCB_CNTMODE_PW_gc;
TCB2.EVCTRL |= TCB_CAPTEI_bm;		// Edge 0 : Rising Edge Counter clear, Falling Edge Capture, int : CAPT INT Enable
TCB2.CTRLA |= TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm;
TCB2.INTCTRL = TCB_CAPT_bm;
// ECHO -> PD0 -> CHANNEL3 -> TCB2
EVSYS.CHANNEL3 = EVSYS_GENERATOR_PORT1_PIN0_gc;	// PD0 Generator
EVSYS.USERTCB2 = EVSYS_CHANNEL_CHANNEL3_gc;

		if ( echoFlag ) {
			echoFlag = false;
			
			DistanceRaw = (float)EchoCounter * 0.1f / 58.0f;	// 20/2 MHz (0.1us)
			DistanceKF  = KalmanFilter( DistanceRaw );
			
			if ( CLCD_Mode == CLCD_ECHO ) {
				sprintf( tBuffer, "Raw    %7.3fcm", DistanceRaw);  IOX_CLCD_DisplayString( 0, 0, tBuffer);
				sprintf( tBuffer, "Kalman %7.3fcm", DistanceKF);   IOX_CLCD_DisplayString( 1, 0, tBuffer);
			}
			if ( FND_Mode == FND_ECHO ) {
				displayUnsignedDecimalPoint( (uint16_t)(DistanceKF*10), 2 );
			}
			
			if ( EchoCount > 0 ) {
				EchoCount--;
				printf("%f %f\n\r", DistanceRaw, DistanceKF);
			}
		}
		
		
		if ( (CntHz & 0b111111) == 0 ) PORTF.OUTSET = PIN5_bm;  else PORTF.OUTCLR = PIN5_bm;	// Trig. 15.6Hz(64ms)
		
		ISR( TCB2_INT_vect ) {
			EchoCounter = TCB2.CCMP;
			echoFlag = true;
			
			TCB2.INTFLAGS |= TCB_CAPT_bm;
		}