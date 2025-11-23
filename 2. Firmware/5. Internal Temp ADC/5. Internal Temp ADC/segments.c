#include <avr/io.h>
#include <avr/interrupt.h>

#include "segments.h"

uint8_t segDigit[4] = {0xff,0xff,0xff,0xff};

const uint8_t segLUT[17] = {
	0x3f, 0x06, 0x5b, 0x4f, 0x66,
	0x6d, 0x7d, 0x27, 0x7f, 0x67,
	0x77, 0x7c, 0x39, 0x5e, 0x79,
	0x71, 0x40
};

/*
 * 앞서 설명한 것 처럼 어느 핀에 다알링톤 회로가 연결 되었는지 생각을 하면서 코드를 작성해야 할 것이다.
 * PORTF.DIRSET = PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm;
 * PORTC.DIR = 0xff;
 *
 * 설정이 완료가 되었다면 main()의 while(1)로 들어가 segSignedDisplay(data);를 만들어준다.
 *
 * callback) main()의 while(1)로 들어가 segSignedDisplay(data);
 */
void SEG_Init(void)
{
	PORTF.DIRSET = PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm;
	PORTC.DIR = 0xff;
}

/*
 * segBuffer[]에 담아 두었다가 한번에 segDigit[]에 넣어서 정보를 전달해줘야한다.
 * 이는 인터럽트에서 발생하는 크리티컬 섹션 때문인데
 * 크리티컬 섹션이란 인터럽트 자원을 main 자원과 공유되었을 때 주로 발생하며
 * CURD 상황에서 발생한다.
 * CURD란 Create Update Read Delete 로 Web 개발자들이 자주 사용하는 용어이다.
 * 
 * 많은 임베디드 개발자들은 크리티컬 섹션을 디버깅 할줄 몰라서 폴링 방식으로 작성을 한다.
 * 이런 개 쓰래기같은 코드는 작성하지 말아야한다.
 * 크리티컬 섹션은 논리적으로 모두 맞는데 발생하는 경우로 아주 골때리는 현상이라고 한다.
 * 이를 디버깅 하기 위해서는 내가 main 에서 사용하는 자원과 interrupt 에서 사용하는 자원을 구분에서 정리를 해보고
 * 크리티컬 섹선이 발생하는 경우가 생길 위험이 있는 자원부터 확인을 해본다.
 * 원인이 된 자원을 찾는게 제일 문제가 크지만
 * 찾았다면 50% 해결한 것과 마찬가지이다.
 * 이 자원에서 main 에서 instruction 이 몇번걸리고 interrupt 에서 몇번 걸리는지 확인을 하여
 * instruction 을 줄이는 방법을 찾아야한다.
 * 몇번의 instruction 이 발생하는지 확인 하려면 lss 리스팅 파일을 확인하는 수 밖에 없다.
 * instruction 을 줄이기 위해 7 segment 에서는 마지막에 segBuffer[]에 담아 두었다가 한번에 segDigit[]에 넣어서 정보를 전달해주는 방식을 채택했다.
 *
 * data 연산과정과 reading zero kill 과 data setting 3단계가 존재한다.
 * data 가 -999, 0, 999 를 기준으로 data 에 어떤 값이 들어가야 하는지 연산을 진행한다.
 * 만약 첫째 자리수가 아닌 다른 자리에서 차례대로 0이 왔을 때를 확인해서 제거해준다.
 * 꼭 else break;를 해줘야한다. 없애고 확인해보자.
 * 
 * 크리티컬 섹션을 없애기 위해 마지막에 데이터를 넣어주는 과정을 한다. 만약 데이터 값이 이상할 경우 여기에 break point를 걸어 segDigit[]에 알맞은 데이터가 들어가는지 확인한다.
 * 
 * 데이터를 넣어주는 과정을 마쳤다. 넣어준 데이터를 Display 하기 위해 Anti Ghost 를 진행하자.
 *
 * goto) ISR(TCB0_INT_vect)
 */
void segSignedDisplay(int16_t data)
{
	uint8_t segBuffer[4];
	
	// segBuffer[0] 및 data 연산
	if(data < -999) { segBuffer[0] = segLUT[16]; data = 999; }
	else if(data >= -999 && data < 0) { segBuffer[0] = segLUT[16]; data = -data; }
	else if(data >= 0 && data <= 999) { segBuffer[0] = 0; }
	else { segBuffer[0] = 0; data = 999; }
	
	segBuffer[1] = segLUT[data/100];
	segBuffer[2] = segLUT[(data%100)/10];
	segBuffer[3] = segLUT[data%10];
	
	for(uint8_t i = 1; i < 3; i++)
	{
		if(segBuffer[i] == 0x3f) segBuffer[i] = 0;
		else break;
	}

	for(uint8_t i = 0; i < 4; i++)
	segDigit[i] = segBuffer[i];
}

void segUnsignedData(uint16_t data)
{
	uint8_t segBuffer[4];
	
	segBuffer[0] = segLUT[data/1000];
	segBuffer[1] = segLUT[(data%1000)/100];
	segBuffer[2] = segLUT[(data%100)/10];
	segBuffer[3] = segLUT[data%10];
	
	// reading zero kill
	for(uint8_t i = 0; i < 3; i++)
	{
		if(segBuffer[i] == 0x3f) segBuffer[i] = 0;
		else break;
	}
	
	for(uint8_t i = 0; i < 4; i++)
	{
		segDigit[i] = segBuffer[i];
	}
}


/*
 * 맨 처음에 세그먼트를 켜고 끄는 동작을 암기하라고 했다.
 * 이를 바탕으로 세그먼트의 동작을 제어해주게된다.
 * 
 * uint8_t index = (uint8_t)(Cnt1000Hz & 0x03);
 * & 기호는 이렇게 생각하면 된다. A에 있는 B 비트를 처다봐라. (A & B)
 * 즉 0bxxxxxx00을 쳐다보게 되고 그 숫자는 00, 01, 10, 11 4개가 되어 segDigit[] 의 자리수임을 확인 할 수 있다.
 * 
 * PORTF.OUTCLR = PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm;
 * 세그먼트를 모두 꺼준다.
 * PORTC.OUT = segDigit[index];
 * 꺼준 상태에서 데이터를 넣어주고 LEF를 순서대로 00 > 01 > 10 > 11 을 켜주게 되면 Ghost 현상이 사라진다.
 * PORTF.OUTSET = 1 << index;
 *
 * 모든 코드를 작성했다. 이제 세그먼트의 동작을 확인하고 임의로 오류를 발생시켜 디버그를 진행해보자.
 */
void segAntiGhostISR(uint16_t Cnt1000Hz)
{
	uint8_t index = (uint8_t)(Cnt1000Hz & 0x03);
	
	PORTF.OUTCLR = PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm;
	PORTC.OUT = segDigit[index];
	PORTF.OUTSET = 1 << index;
}