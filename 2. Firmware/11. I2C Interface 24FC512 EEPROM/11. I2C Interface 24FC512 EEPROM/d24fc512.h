#ifndef D24FC512_H_
#define D24FC512_H_

/*
 * #EEPROM_D24FC512
 * #I2C #PageWrite #AnyBlockWrite
 *
 * 본 헤더 파일은 외부 EEPROM(D24FC512, 512Kbit = 64Kbyte)에 접근하기 위한
 * 기본적인 Read/Write 함수들의 원형을 제공한다.
 *
 * EEPROM은 내부적으로 128byte 단위의 페이지(Page)로 구성되어 있으며,
 * Page Write를 사용할 경우 반드시 같은 Page 안에서만 연속 저장이 가능하다.
 * 만약 Page 경계를 넘으면 EEPROM 내부 동작에 의해 Page의 첫 주소로 되돌아가
 * 기존 데이터를 덮어쓰게 되므로(Page Wrap-Around), 주의가 필요하다.
 *
 * 따라서 실제로 큰 데이터(구조체, 버퍼 등)를 저장할 때는
 * 페이지 경계를 자동으로 처리하는 EEPROM_WriteAnyBlock()을 사용하는 것을 권장한다.
 *
 * callback) 아래 각 함수의 용도 설명을 보자.
 */

#define D24FC512_SLAVE_ADDRESS   0x50
/*
 * EEPROM의 I2C 기본 Slave Address.
 * 상위 주소 비트(A16~A8)는 Device Address LSB 3비트에 매핑된다.
 *
 * 예시)
 * 실제 접근 주소가 0x1234 라면,
 * Device Address = 0x50 | ((address >> 8) & 0x07)
 * Word Address   = address & 0xFF
 */

#define PAGE_NO                  128
/*
 * D24FC512의 내부 Page Size = 128 byte
 *
 * EEPROM 내부는 아래와 같이 128byte 단위 Page로 구분되어 있다.
 * 
 * Page 0 : 0x0000 ~ 0x007F (128 byte)
 * Page 1 : 0x0080 ~ 0x00FF
 * Page 2 : 0x0100 ~ 0x017F
 * Page 3 : 0x0180 ~ 0x01FF
 * ...
 *
 * Page Write 사용 시 반드시 이 128byte 범위 내부에서만 연속 저장해야 한다.
 * Page 경계를 넘으면 EEPROM이 자동으로 Page 시작 주소로 돌아가서 덮어쓴다.
 * (Page Wrap-Around 발생)
 *
 * callback) D24FC512_Write_Address_Page() 설명을 보자.
 */


/* ======================================================================
   1. Byte 단위 Read/Write
   ====================================================================== */

/*
 * #ByteWrite
 * #SingleWrite
 *
 * 특정 EEPROM 주소에 1byte를 기록한다.
 * Page 관계 없이 어느 위치에나 저장 가능하지만,
 * Write Cycle(5~10ms)이 발생하므로 연속 기록 시 속도가 느릴 수 있다.
 */
void D24FC512_Write_Address_Uint8(uint16_t address, uint8_t data);

/*
 * #ByteRead
 * 지정된 EEPROM 주소에서 1byte를 읽어온다.
 */
uint8_t D24FC512_Read_Address_Uint8(uint16_t address);


/* ======================================================================
   2. 16bit Read/Write (2 byte 연속 접근)
   ====================================================================== */

/*
 * #Uint16Write
 *
 * 16bit 값을 2byte로 나누어 address, address+1 에 연속으로 기록한다.
 * 내부적으로 Byte Write를 2회 수행하거나, Page 내부라면 Page Write로 구현될 수 있다.
 * 구조체 저장의 기본 단위로도 사용된다.
 */
void D24FC512_Write_Address_Uint16(uint16_t address, uint16_t data);

/*
 * #Uint16Read
 *
 * 16bit 값(2byte)을 연속해서 읽어 하나의 uint16_t 값으로 반환한다.
 */
uint16_t D24FC512_Read_Address_Uint16(uint16_t address);


/* ======================================================================
   3. Page Write / Page-aware Write
   ====================================================================== */

/*
 * #PageWrite
 * #128byte_Page
 *
 * Page 내부에 한정하여 연속 Write를 수행한다.
 * length 만큼의 데이터를 buffer로부터 읽어 address 위치부터 저장한다.
 *
 * 단, 중요한 점은 반드시 "같은 Page(128byte) 내부"에서만 호출해야 한다.
 *
 * 예)
 * address = 0x0100, length = 10 → 정상 (Page 2 내부)
 * address = 0x017F, length = 10 → 첫 1byte만 쓰고 Page 시작(0x0100)으로 돌아가 9byte 덮어씀 (위험!)
 *
 * 따라서 긴 데이터 저장에는 사용하지 않고,
 * Page 경계를 넘지 않는 작은 버퍼 저장 시에만 사용한다.
 *
 * callback) EEPROM_WriteAnyBlock()으로 Page 경계 문제를 해결할 수 있다.
 */
void D24FC512_Write_Address_Page(uint16_t address, uint8_t *buffer, uint8_t length);


/*
 * #AnyBlockWrite
 * #AutoPageSplit
 *
 * 페이지 경계를 자동으로 계산하여, 어떤 주소에서든 length 만큼의 데이터를
 * 안전하게 EEPROM에 기록한다.
 *
 * 내부적으로:
 *   1) 현재 Page에 남은 공간 계산
 *   2) Page 내부 가능한 범위까지 기록 (D24FC512_Write_Address_Page 사용)
 *   3) 다음 Page로 넘어가 남은 길이 반복 저장
 *
 * 즉, 개발자가 Page 구조를 의식할 필요 없이 아무 데이터나 저장할 수 있게 해준다.
 * 실제 프로젝트에서는 대부분 이 함수를 사용한다.
 */
void EEPROM_WriteAnyBlock(uint16_t address, uint8_t *buffer, uint16_t length);


/* ======================================================================
   4. Block Read
   ====================================================================== */

/*
 * #BlockRead
 *
 * EEPROM의 특정 주소부터 length 바이트만큼 연속 읽기.
 * 읽기는 Page 경계와 관계없이 수행 가능하므로 사용이 간단하다.
 *
 * 예)
 * 구조체 저장 시 sizeof(struct) 만큼 블록 전체를 읽어올 때 사용.
 */
void D24FC512_Read_Address_Block(uint16_t address, uint8_t *buffer, uint16_t length);


#endif /* D24FC512_H_ */
