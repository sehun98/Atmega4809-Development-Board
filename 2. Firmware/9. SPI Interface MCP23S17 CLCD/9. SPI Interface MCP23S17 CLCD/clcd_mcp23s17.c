#define F_CPU 5000000UL

#include <util/delay.h>
#include "spi.h"
#include "mcp23s17.h"
#include "clcd_mcp23s17.h"

// LCD 핀 정의 (MCP23S17 GPIOA)
#define LCD_RS   1
#define LCD_RW   2
#define LCD_EN   3
#define LCD_D4   4
#define LCD_D5   5
#define LCD_D6   6
#define LCD_D7   7

// 내부 캐시
static uint8_t g_gpioa_cache = 0x00;

// -------------------------------
// Low-level GPIO control
// -------------------------------
static inline void CLCD_SetPin(uint8_t pin)
{
	g_gpioa_cache |= (1 << pin);
	MCP23S17_WriteGPIOA(g_gpioa_cache);
}

static inline void CLCD_ClearPin(uint8_t pin)
{
	g_gpioa_cache &= ~(1 << pin);
	MCP23S17_WriteGPIOA(g_gpioa_cache);
}

// -------------------------------
// Enable pulse
// -------------------------------
static void CLCD_PulseEnable(void)
{
	CLCD_SetPin(LCD_EN);
	_delay_us(1);
	CLCD_ClearPin(LCD_EN);
	_delay_us(50);   // LCD latch time
}

// -------------------------------
// Send only upper 4 bits (Init Mode)
// -------------------------------
static void CLCD_SendUpperNibble(uint8_t cmd)
{
	uint8_t nib = (cmd >> 4) & 0x0F;

	// D4~D7 clear
	g_gpioa_cache &= ~((1<<LCD_D4)|(1<<LCD_D5)|(1<<LCD_D6)|(1<<LCD_D7));

	if (nib & 0x01) g_gpioa_cache |= (1<<LCD_D4);
	if (nib & 0x02) g_gpioa_cache |= (1<<LCD_D5);
	if (nib & 0x04) g_gpioa_cache |= (1<<LCD_D6);
	if (nib & 0x08) g_gpioa_cache |= (1<<LCD_D7);

	MCP23S17_WriteGPIOA(g_gpioa_cache);
	CLCD_PulseEnable();
}

// -------------------------------
// Normal 4-bit nibble send
// -------------------------------
static void CLCD_Send4Bits(uint8_t nibble)
{
	g_gpioa_cache &= ~((1<<LCD_D4)|(1<<LCD_D5)|(1<<LCD_D6)|(1<<LCD_D7));

	if (nibble & 0x01) g_gpioa_cache |= (1<<LCD_D4);
	if (nibble & 0x02) g_gpioa_cache |= (1<<LCD_D5);
	if (nibble & 0x04) g_gpioa_cache |= (1<<LCD_D6);
	if (nibble & 0x08) g_gpioa_cache |= (1<<LCD_D7);

	MCP23S17_WriteGPIOA(g_gpioa_cache);
	CLCD_PulseEnable();
}

// -------------------------------
// Instruction / Data
// -------------------------------
static void IOX_CLCD_SendInst(uint8_t dat)
{
	CLCD_ClearPin(LCD_RS);
	CLCD_ClearPin(LCD_RW);

	CLCD_Send4Bits(dat >> 4);
	CLCD_Send4Bits(dat & 0x0F);

	_delay_us(40);
}

static void IOX_CLCD_SendData(uint8_t dat)
{
	CLCD_SetPin(LCD_RS);
	CLCD_ClearPin(LCD_RW);

	CLCD_Send4Bits(dat >> 4);
	CLCD_Send4Bits(dat & 0x0F);

	_delay_us(40);
}

// -------------------------------
// Public API
// -------------------------------
void IOX_CLCD_Init(void)
{
	_delay_ms(40);

	CLCD_ClearPin(LCD_RS);
	CLCD_ClearPin(LCD_RW);

	// 8bit wake-up
	CLCD_SendUpperNibble(0x30);
	_delay_ms(5);

	CLCD_SendUpperNibble(0x30);
	_delay_us(160);

	CLCD_SendUpperNibble(0x30);
	_delay_us(160);

	// 4bit entry
	CLCD_SendUpperNibble(0x20);
	_delay_us(160);

	// Normal 4bit commands
	IOX_CLCD_SendInst(0x28);  // 4bit, 2line, 5x8
	IOX_CLCD_SendInst(0x0C);  // display on
	IOX_CLCD_SendInst(0x01);  // clear
	_delay_ms(2);
	IOX_CLCD_SendInst(0x06);  // entry mode
}


void IOX_CLCD_Clear(void)
{
	IOX_CLCD_SendInst(0x01);
	_delay_ms(2);
}

void IOX_CLCD_GotoRC(uint8_t row, uint8_t col)
{
	col %= 16;
	row %= 2;

	uint8_t addr = (row == 0) ? col : 0x40 + col;
	IOX_CLCD_SendInst(0x80 | addr);
}

void IOX_CLCD_DisplayChar(uint8_t chr)
{
	IOX_CLCD_SendData(chr);
}

void IOX_CLCD_DisplayString(uint8_t r, uint8_t c, char *str)
{
	IOX_CLCD_GotoRC(r, c);

	while (*str)
	IOX_CLCD_SendData(*str++);
}

void IOX_CLCD_makeFont(uint8_t addr, uint8_t *user_font)
{
	IOX_CLCD_SendInst(0x40 + addr * 8);

	for (uint8_t i = 0; i < 8; i++)
	IOX_CLCD_SendData(user_font[i]);
}
