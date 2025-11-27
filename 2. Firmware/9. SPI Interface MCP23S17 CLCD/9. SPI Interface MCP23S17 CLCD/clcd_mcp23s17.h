#ifndef CLCD_MCP23S17_H_
#define CLCD_MCP23S17_H_

#include <stdint.h>

void IOX_CLCD_Init(void);

void IOX_CLCD_Clear(void);
void IOX_CLCD_GotoRC(uint8_t row, uint8_t col);

void IOX_CLCD_DisplayString(uint8_t r, uint8_t c, char *str);
void IOX_CLCD_DisplayChar(uint8_t chr);

void IOX_CLCD_makeFont(uint8_t addr, uint8_t *user_font);

#endif /* CLCD_MCP23S17_H_ */
