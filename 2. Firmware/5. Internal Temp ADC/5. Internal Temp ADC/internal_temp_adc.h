#ifndef INTERNAL_TEMP_ADC_H_
#define INTERNAL_TEMP_ADC_H_

#include <avr/io.h>
#include <avr/interrupt.h>

void ADC0_Init(void);

int8_t sigrow_offset;
uint8_t sigrow_gain;
extern uint16_t temperature_in_K;

#endif /* INTERNAL_TEMP_ADC_H_ */