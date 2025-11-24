#define F_CPU	20000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include "cds_cell_adc.h"

int main(void)
{
	InitializeADC();
	
	// TCB0 CAPT -> CHANNEL0 -> ADC0 SOC
	EVSYS.CHANNEL0 = EVSYS_GENERATOR_TCB0_CAPT_gc;
	EVSYS.USERADC0 = EVSYS_CHANNEL_CHANNEL0_gc;
	
	sei();
	
    while (1) 
    {
		if ( rawReadyFlag ) {
			rawReadyFlag = false;
			
			MA_Filter(&AdcResult);
			
			if(filteredReadyFlag) {
				filteredReadyFlag = false;
				printf("%d\n\r", AdcResult.filteredData);
			}
		}
    }
}

