#include "internal_temp_adc.h"

int8_t sigrow_offset;
uint8_t sigrow_gain;
uint16_t temperature_in_K = 0;

/*
 * #ADCInitialization
 * #VREF #MUXPOS #SIGROW
 *
 * VREF.CTRLA = 1.1V 내부 참조 전압 선택  
 * VREF.CTRLB = INTREF enable → ADC에서 내부 참조 사용 가능
 *
 * ADC0.CTRLD |= ADC_INITDLY_DLY32_gc  
 *  → ADC 시작 전 안정화 지연(Init Delay). TEMPSENSE 사용 시 권장 값.
 *
 * ADC0.MUXPOS = ADC_MUXPOS_TEMPSENSE_gc  
 *  → 일반적인 ADC 입력 대신 내부 온도센서를 선택
 *
 * ADC0.CTRLC 설정  
 *  - ADC_PRESC_DIV16_gc : 클럭 분주
 *  - ADC_REFSEL_INTREF_gc : 내부 1.1V 참조
 *  - ADC_SAMPCAP_bm : High sampling capacitance enable
 *
 * SIGROW.TEMPSENSE0(TS_GAIN), SIGROW.TEMPSENSE1(TS_OFFSET)  
 *  → 공장에서 저장한 온도센서 보정값이다.
 *
 * 마지막으로 ADC0.INTCTRL |= ADC_RESRDY_bm  
 * → 변환 완료 시 인터럽트 발생하도록 한다.
 *
 * goto) ISR(ADC0_RESRDY_vect)에서 계산 과정을 보자.
 */
void ADC0_Init(void)
{
	// 내부 1.1V Reference 사용
	VREF.CTRLA |= VREF_ADC0REFSEL_1V1_gc;
	VREF.CTRLB |= VREF_ADC0REFEN_bm;
	
	// TEMPSENSE 사용 시 안정화를 위한 Delay
	ADC0.CTRLD |= ADC_INITDLY_DLY32_gc;
	
	// MUX 입력을 Temperature Sensor로 설정
	ADC0.MUXPOS |= ADC_MUXPOS_TEMPSENSE_gc;
	
	// Prescaler, Reference Source, Sample Capacitance 설정
	ADC0.CTRLC |= ADC_PRESC_DIV16_gc | ADC_REFSEL_INTREF_gc | ADC_SAMPCAP_bm;
	// 샘플 횟수 설정 (datasheet 권장값)
	ADC0.SAMPCTRL |= 32;
	
	// Event 입력 발생 시 ADC 변환 시작 (사용자 시스템 의존)
	ADC0.EVCTRL |= ADC_STARTEI_bm;
	
	// 공장에서 보정한 Calibration 값 불러오기
	sigrow_offset = SIGROW.TEMPSENSE1;
	sigrow_gain = SIGROW.TEMPSENSE0;
	
	// ADC Enable + Interrupt Enable
	ADC0.CTRLA |= ADC_ENABLE_bm;
	ADC0.INTCTRL |= ADC_RESRDY_bm;
}

/*
 * #ADCInterruptRoutine
 * #FactoryCalibration #TemperatureCalculation
 *
 * 내부 온도 센서를 사용하는 경우 단순히 ADC0.RES 값을 그대로 사용하면 안 된다.
 * 공장에서 저장한 offset, gain 값을 적용해야 실제 온도에 근접한 값이 된다.
 *
 * temp = adc_reading - sigrow_offset;
 * temp *= sigrow_gain;
 * temp += 0x80;       // rounding
 * temp >>= 8;         // scale-down
 *
 * datasheet 공식식에 따라 Kelvin 값으로 산출한다.
 *
 * 주의:  
 * - ISR 내부에서는 최소한의 계산만 하고, flag 또는 저장만 수행해야 한다.
 * - 시간 소모적인 floating 연산은 절대 ISR에서 하지 않도록 한다.
 *
 * goto) main()에서 temperature_in_K를 사용해보자.
 */
ISR(ADC0_RESRDY_vect)
{
	uint16_t adc_reading;
	uint32_t temp;
	
	// 변환 결과 읽기
	adc_reading = ADC0.RES;
	
	// Factory Calibration 공식
	temp = adc_reading - sigrow_offset;
	temp *= sigrow_gain;
	temp += 0x80;
	temp >>= 8;
	
	// Kelvin 값 저장
	temperature_in_K = temp;
	
	// 인터럽트 플래그 클리어
	ADC0.INTFLAGS |= ADC_RESRDY_bm;
}