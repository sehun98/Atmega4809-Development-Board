#define F_CPU 5000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdbool.h>
#include <util/delay.h>
#include "i2c.h"
#include "d24fc512.h"

void D24FC512_Write_Address_Uint8(uint16_t address, uint8_t data)
{
	I2C_Write_Address_Uint8(D24FC512_SLAVE_ADDRESS,address,data);
}

void D24FC512_Write_Address_Uint16(uint16_t address, uint16_t data)
{
	I2C_Write_Address_Uint16(D24FC512_SLAVE_ADDRESS,address,data);
}

void D24FC512_Write_Address_Page(uint16_t address, uint8_t *buffer, uint8_t length)
{
    uint16_t pageStart, pageNext;

    /*
     * pageStart :
     *   address가 속한 Page의 시작 주소를 구한다.
     *   예) address = 0x0137 → Page 2 → pageStart = 0x0100
     *
     *  address & ~(PAGE_NO - 1)
     *  = address & ~(128 - 1)
     *  = address & 0xFF80
     *  즉, 아래 7bit를 모두 0으로 만들어 Page 시작 주소 계산
     */
    pageStart = address & ~(PAGE_NO - 1);

    /*
     * pageNext :
     *   현재 Page 끝 다음 주소 (다음 Page의 시작)
     *   pageStart + 128
     */
    pageNext = pageStart + PAGE_NO;

    /*
     * (address + length) <= pageNext
     *
     * 현재 address에서 length 만큼의 데이터가
     * 이 Page 내부에서 모두 저장 가능한지 판단하는 조건.
     *
     * 가능하면 그대로 전체 길이를 저장하고,
     * 불가능하면 Page 경계까지의 범위만 저장한다.
     */
    if ((address + length) <= pageNext) {
        I2C_Write_Address_Block((i2c_address_t)D24FC512_SLAVE_ADDRESS, address, buffer, (uint16_t)length);
    }
    else {
        /* Page 경계를 넘기 때문에 pageNext - address 만큼만 저장 */
        I2C_Write_Address_Block((i2c_address_t)D24FC512_SLAVE_ADDRESS, address, buffer, pageNext - address);
    }

    /*
     * EEPROM은 내부 Write Cycle이 있으며 5~10ms가 필요하다.
     * 이 시간 동안 Busy 상태이기 때문에 다음 Write를 바로 하면 실패한다.
     * 따라서 반드시 delay를 걸어 Write Cycle이 완료되도록 한다.
     */
    _delay_ms(5);
}

/*
 * #AnyBlockWrite
 * #AutoPageSplit
 *
 * EEPROM_WriteAnyBlock()
 *
 * 이 함수는 Page Write의 가장 큰 단점인 "Page 경계 문제"를 해결한다.
 *
 * Page Write는 128byte Page 내부에서는 자유롭게 저장 가능하지만,
 * Page 경계를 넘으면 Wrap-Around가 발생해 데이터가 파괴된다.
 *
 * 이 문제를 해결하기 위해 AnyBlock 함수는 아래의 순서로 동작한다:
 *
 *  1) 처음 address가 위치한 Page에서 남은 공간 first_siz 계산
 *     (첫 Page에서 저장 가능한 최대 크기)
 *
 *  2) first_siz보다 length가 크면
 *       - 첫 Page에 first_siz 만큼 저장
 *       - 남은 length를 다음 Page부터 이어서 저장
 *
 *  3) PAGE_NO(128byte) 단위로 block_no 만큼 반복 저장
 *
 *  4) 마지막 Page에 length가 남아있으면 마저 저장
 *
 * 이렇게 하면 아무 길이의 데이터라도 Page 경계를 넘겨가며
 * 안전하게 저장할 수 있다.
 *
 * 즉, 이 함수는 페이지 개념을 “자동으로 해결”해주기 때문에
 * 구조체 저장, 대용량 버퍼 저장, 설정값 저장 등 대부분의 상황에서
 * 가장 실전적이고 안정적으로 사용되는 Write 함수이다.
 *
 * callback) PageWrite와 BlockWrite 차이를 다시 복습하자.
 */
void EEPROM_WriteAnyBlock(uint16_t address, uint8_t *buffer, uint16_t length)
{
    uint16_t pageStart, pageNext, first_siz, block_no;

    /*
     * pageStart = 현재 주소의 Page 시작 주소
     * pageNext  = 다음 Page의 시작 주소
     * first_siz = 현재 Page에서 남아있는 빈 공간
     */
    pageStart = address & ~(PAGE_NO - 1);
    pageNext = pageStart + PAGE_NO;
    first_siz = pageNext - address;

    /*
     * length > first_siz
     * 첫 Page 안에 length 전체가 들어가지 않을 경우
     * → first_siz 만큼 먼저 쓰고 다음 Page로 넘긴다
     */
    if (length > first_siz)
    {
        /* 첫 Page에 넣을 수 있는 만큼만 기록 */
        D24FC512_Write_Address_Page(address, buffer, (uint8_t)first_siz);

        /* Page 넘어갈 준비 */
        length -= first_siz;
        buffer += first_siz;
        address += first_siz;

        /*
         * block_no = length / 128
         * 남은 데이터가 128byte Page 단위로 몇 블록인지 계산
         */
        block_no = length / PAGE_NO;

        /* 중간의 완전한 Page 블록들을 반복 저장 */
        if (block_no) {
            for (uint16_t i = 0; i < block_no; i++) {
                D24FC512_Write_Address_Page(address, buffer, (uint8_t)PAGE_NO);

                length -= PAGE_NO;
                buffer += PAGE_NO;
                address += PAGE_NO;
            }
        }

        /*
         * 마지막 Page에 남은 length가 있다면 저장
         */
        if (length) {
            D24FC512_Write_Address_Page(address, buffer, (uint8_t)length);
        }
    }
    else {
        /* length가 첫 Page 안에서 끝나는 경우 */
        D24FC512_Write_Address_Page(address, buffer, (uint8_t)length);
    }
}

//////////////////////////////////////////////////////////////////////////
uint8_t D24FC512_Read_Address_Uint8(uint16_t address)
{
	return I2C_Read_Address_Uint8(D24FC512_SLAVE_ADDRESS, address);
}
uint16_t D24FC512_Read_Address_Uint16(uint16_t address)
{
	return I2C_Read_Address_Uint16(D24FC512_SLAVE_ADDRESS, address);
}
void D24FC512_Read_Address_Block(uint16_t address, uint8_t *buffer, uint16_t length)
{
	I2C_Read_Address_Block(D24FC512_SLAVE_ADDRESS, address, buffer, length);
}