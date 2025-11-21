#ifndef ROTARY_H_
#define ROTARY_H_

#include <avr/io.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <stdbool.h>

#define		RotSW_Key		PIN1_bm
#define		RotSW_S1		PIN2_bm
#define		RotSW_S2		PIN3_bm
#define		ROT_BUTTON		(PORTE.IN & RotSW_Key)

extern volatile	bool	rotDirectionFlag;
extern volatile	bool	rotSwitchFlag;
extern volatile	int16_t	buttonCount;

typedef enum { ROT_IDLE, ROT_PRESSING, ROT_PRESSED, ROT_RELEASE } RotSwState_t;
typedef enum { S00, S01, S10, S11 } RotState_t;

void ROT_Init(void);
uint8_t GetRotSwitch(void);
void RotSwitchISR(void);
void RotDirectionISR(void);

#endif /* ROTARY_H_ */