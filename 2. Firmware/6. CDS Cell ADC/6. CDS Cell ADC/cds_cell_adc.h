#ifndef CDS_CELL_ADC_H_
#define CDS_CELL_ADC_H_

#define MA_MAX_SIZE 32

struct adc_info {
	uint16_t	rawData;
	uint16_t	filteredData;
	uint8_t		subSample;
	uint8_t		cntSample;
	
	uint8_t		MA_Size;
	uint8_t		MA_Index;
	uint32_t	MA_Sum;
	uint16_t	*pMA_Buffer;
};

extern volatile bool rawReadyFlag;
extern volatile bool filteredReadyFlag;
extern struct adc_info AdcResult;

void InitializeADC( void );
void MA_Filter(struct adc_info *);

#endif /* CDS_CELL_ADC_H_ */