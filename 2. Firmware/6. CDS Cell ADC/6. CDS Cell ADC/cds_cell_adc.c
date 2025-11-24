#define F_CPU 20000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "cds_cell_adc.h"

volatile bool rawReadyFlag = false;
volatile bool filteredReadyFlag = false;

static uint16_t MA_Buffer[MA_MAX_SIZE];

struct adc_info AdcResult = {
	.rawData = 0,
	.filteredData = 0,
	.subSample = 10,
	.cntSample = 0,
	
	.MA_Size = 16,
	.MA_Index = 0,
	.MA_Sum = 0,
	.pMA_Buffer = MA_Buffer
};

ISR(ADC0_RESRDY_vect)
{
	AdcResult.rawData = ADC0.RES;
	rawReadyFlag = true;
	
	ADC0.INTFLAGS = ADC_RESRDY_bm;
}

void InitializeADC(void)
{
	for (uint8_t i = 0; i < MA_MAX_SIZE; i++) MA_Buffer[i] = 0;

	AdcResult.MA_Sum = 0;

	VREF.CTRLA |= VREF_ADC0REFSEL_2V5_gc;
	VREF.CTRLB |= VREF_ADC0REFEN_bm;

	ADC0.CTRLC |= ADC_REFSEL_INTREF_gc | ADC_PRESC_DIV64_gc | ADC_SAMPCAP_bm;
	ADC0.MUXPOS = ADC_MUXPOS_AIN1_gc;

	ADC0.EVCTRL = ADC_STARTEI_bm;

	ADC0.CTRLA |= ADC_ENABLE_bm;
	ADC0.INTCTRL |= ADC_RESRDY_bm;
}

void MA_Filter(struct adc_info *adc)
{
	// Sub-sampling
	if (++adc->cntSample >= adc->subSample)
	{
		adc->cntSample = 0;

		if (adc->MA_Size > 0)
		{
			adc->MA_Sum -= adc->pMA_Buffer[adc->MA_Index];
			adc->MA_Sum += adc->rawData;
			adc->pMA_Buffer[adc->MA_Index] = adc->rawData;

			adc->MA_Index++;
			if (adc->MA_Index >= adc->MA_Size)
			adc->MA_Index = 0;

			adc->filteredData = (uint16_t)(adc->MA_Sum / (uint32_t)adc->MA_Size);
		}
		else
		{
			adc->filteredData = adc->rawData;
		}

		filteredReadyFlag = true;   // 필터링된 값 준비됨
	}
}