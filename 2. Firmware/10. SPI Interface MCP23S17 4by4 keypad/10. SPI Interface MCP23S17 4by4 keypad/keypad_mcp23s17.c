#include "keypad_mcp23s17.h"
#include "mcp23s17.h"
#include <util/delay.h>

volatile uint8_t KeySwCol = 0xFF;
volatile uint8_t gkswScanCode = 0xFF;
volatile bool    gkswFlag = false;

#define ROW1_MASK   0xE0   // 1110 0000
#define ROW2_MASK   0xD0   // 1101 0000
#define ROW3_MASK   0xB0   // 1011 0000
#define ROW4_MASK   0x70   // 0111 0000

typedef enum {
    kswILDE0, kswILDE1,
    kswPRESSING,
    kswScanRow0, kswScanRow1, kswScanRow2, kswScanRow3,
    kswPRESSED, kswRELEASING
} KeySW_State_t;

// ---------------- ROW 선택 + COL 읽기 ----------------
static void getKeySW(uint8_t rowMask)
{
    uint8_t out = (rowMask & 0xF0) | 0x0F;  // row 설정 + col = 1111

    MCP23S17_WriteReg(IOX_GPIOB, out);
    _delay_us(5);

    uint8_t val = MCP23S17_ReadReg(IOX_GPIOB);
    KeySwCol = val & 0x0F;
}


// ---------------- FSM -----------------
void ScanKeySwISR(void)
{
    static KeySW_State_t State = kswILDE0;
    uint8_t col, i;

    switch (State)
    {
        case kswILDE0:
            getKeySW(0xF0);
            State = kswILDE1;
            break;

        case kswILDE1:
            if ((KeySwCol & 0x0F) != 0x0F) State = kswPRESSING;
            getKeySW(0xF0);
            break;

        case kswPRESSING:
            if ((KeySwCol & 0x0F) != 0x0F)
            {
                State = kswScanRow0;
                getKeySW(ROW1_MASK);
            }
            else
            {
                State = kswILDE1;
                getKeySW(0xF0);
            }
            break;

        case kswScanRow0:
            col = KeySwCol;
            if (col == 0x0F)
            {
                State = kswScanRow1;
                getKeySW(ROW2_MASK);
            }
            else
            {
                for (i=0; i<4; i++)
                {
                    if (!(col & (1<<i)))
                    {
                        gkswScanCode = i;
                        gkswFlag = true;
                        State = kswPRESSED;
                        getKeySW(0xF0);
                        break;
                    }
                }
            }
            break;

        case kswScanRow1:
            col = KeySwCol;
            if (col == 0x0F)
            {
                State = kswScanRow2;
                getKeySW(ROW3_MASK);
            }
            else
            {
                for (i=0;i<4;i++)
                {
                    if (!(col & (1<<i)))
                    {
                        gkswScanCode = i + 4;
                        gkswFlag = true;
                        State = kswPRESSED;
                        getKeySW(0xF0);
                        break;
                    }
                }
            }
            break;

        case kswScanRow2:
            col = KeySwCol;
            if (col == 0x0F)
            {
                State = kswScanRow3;
                getKeySW(ROW4_MASK);
            }
            else
            {
                for (i=0;i<4;i++)
                {
                    if (!(col & (1<<i)))
                    {
                        gkswScanCode = i + 8;
                        gkswFlag = true;
                        State = kswPRESSED;
                        getKeySW(0xF0);
                        break;
                    }
                }
            }
            break;

        case kswScanRow3:
            col = KeySwCol;
            if (col != 0x0F)
            {
                for (i=0;i<4;i++)
                {
                    if (!(col & (1<<i)))
                    {
                        gkswScanCode = i + 12;
                        gkswFlag = true;
                        break;
                    }
                }
            }
            else
            {
                gkswScanCode = 0xFF;
                gkswFlag = true;
            }
            State = kswPRESSED;
            getKeySW(0xF0);
            break;

        case kswPRESSED:
            if ((KeySwCol & 0x0F) == 0x0F)
                State = kswRELEASING;
            getKeySW(0xF0);
            break;

        case kswRELEASING:
            if ((KeySwCol & 0x0F) != 0x0F)
                State = kswPRESSED;
            else
                State = kswILDE1;
            getKeySW(0xF0);
            break;
    }
}
