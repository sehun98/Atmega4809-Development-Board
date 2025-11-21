#ifndef SEGMENTS_H_
#define SEGMENTS_H_

#define F_CPU 5000000UL

extern void SEG_Init(void);
extern void segUnsignedData(uint16_t);
extern void segSignedDisplay(int16_t);
extern void segAntiGhostISR(uint16_t);

extern uint8_t segDigit[4];
extern const uint8_t segLUT[17];

#endif /* 7SEGMENTS_H_ */