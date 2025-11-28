#define F_CPU 5000000UL

/*
 * #I2C #EEPROM #D24FC512
 * #SignCheck #BootSequence
 *
 * 본 장에서는 외부 EEPROM(D24FC512)에 접근하여
 * 시스템 최초 동작(Sign)을 확인하고,
 * 정상적인 값이 없을 경우 초기화 값을 기록하는 예제를 다룬다.
 *
 * EEPROM은 단순히 읽기/쓰기 외에도
 * "시스템 상태 유지" 또는 "부팅 시 설정값 로드" 같은 역할을 수행한다.
 *
 * 본 예제에서는 Sign(0xAA55)을 EEPROM에 저장하여
 * 시스템이 첫 실행인지, 이미 저장된 값이 있는지를 판단한다.
 *
 * Sign 값이 존재하면 버튼 값(buttonCount 등)을 읽고,
 * Sign 값이 없으면 EEPROM을 초기 상태로 구성한다.
 *
 * EEPROM Address	Stored Byte
 * 0x00	0xAA
 * 0x01	0x55
 *
 * callback) main() 내부의 Sign Check 부분을 먼저 보자.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdbool.h>
#include <util/delay.h>

#include "i2c.h"
#include "d24fc512.h"
#include "uart.h"

/*
 * EEPROM 내부 주소 구성
 *
 * D24F512_SIGN_ADDRESS     : 시스템 부팅 Sign 저장 (2 byte)
 * D24F512_BUTTON_ADDRESS   : 버튼 Count 저장 (2 byte)
 *
 * EEPROM은 주소 단위로 Read/Write가 가능하므로
 * 프로젝트 요구사항에 따라 구조를 자유롭게 설계할 수 있다.
 *
 * callback) main()에서 Read/Write 동작을 보자.
 */
#define D24F512_SIGN_ADDRESS    0x00   // 0x00 ~ 0x01

void CLK_Init(void);

/*
 * #MainRoutine
 * #EEPROM_SignCheck
 *
 * 1) I2C_Init()  
 *    → 외부 EEPROM 접근을 위한 TWI/I2C 초기화.
 *
 * 2) EEPROM Sign 값 확인  
 *    - Sign 값이 0xAA55 라면 이미 시스템이 부팅된 적이 있다는 의미이다.
 *      → 버튼 값을 EEPROM에서 읽어온다.
 *
 *    - Sign 값이 다르면 (초기값이거나, 메모리 초기화된 상태)
 *      → Sign(0xAA55) 값을 쓰고, 버튼 값을 0x00으로 초기화한다.
 *
 * EEPROM은 전원이 꺼져도 값이 유지되므로
 * MCU가 재부팅되거나 전원이 꺼져도 설정값/카운트값을 보존할 수 있다.
 *
 * callback) D24FC512_Read_Address_Uint16(), D24FC512_Write_Address_Uint16()을 참고하자.
 */
int main(void)
{
	CLK_Init();
	
    I2C_Init();
	
	USART0_Init(115200);
	
	sei();
	printf("register memory!!\r\n");
	printf("0x00 : %x\r\n",D24FC512_Read_Address_Uint8(0x00));
	printf("0x01 : %x\r\n",D24FC512_Read_Address_Uint8(0x01));
	printf("0x02 : %x\r\n",D24FC512_Read_Address_Uint8(0x02));
	printf("0x03 : %x\r\n",D24FC512_Read_Address_Uint8(0x03));
	// Write 이후에 _delay_ms(5) 가 필요하다.
	
    if (D24FC512_Read_Address_Uint16(D24F512_SIGN_ADDRESS) == 0xaa55)
    {
		// 저장된 버튼 값 읽기
		printf("Already Exist!!\r\n");
		printf("D24F512_SIGN_ADDRESS : %x\r\n",D24FC512_Read_Address_Uint16(D24F512_SIGN_ADDRESS));
    }
    else
    {
		// Sign 값 신규 기록
		printf("Empty Memory!!\r\n");
	    D24FC512_Write_Address_Uint16(D24F512_SIGN_ADDRESS, 0xaa55);
    }
    while (1) { }
}

void CLK_Init(void)
{
    CCP = CCP_IOREG_gc;  
    CLKCTRL.MCLKCTRLB = CLKCTRL_PDIV_4X_gc | CLKCTRL_PEN_bm;
}
