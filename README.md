# Atmega4809 Custom Development Board

## 1. Introduction
본 프로젝트는 8-bit MCU의 동작 원리를 기초부터 학습하기 위해 진행되었습니다.  
Atmega4809 기반의 커스텀 개발 보드를 설계하여 GPIO, 타이머, ADC, UART 등 MCU의 핵심 기능을 실습형으로 익혔습니다.

## 2. Features
- Atmega4809 기반 2-layer PCB
- 안정적인 5V/3.3V 전원회로 포함
- MCU / TEMP / CDS SENSOR / STRAIN_GAUGE
- UART & 7 Segment Display
- Stepper Motor, Ultrasonic, Rotary Encoder, EEPROM
- IO Expander, LCD, Keypad (SPI)

## 3. Firmware Structure
- AVR-GCC 기반 빌드
- 레지스터 접근 기반 GPIO/ADC/UART/PWM 구현

## 4. Development Environment
- Microchip Studio : Atmega4809 펌웨어 개발 및 디버깅 환경
- entor Graphics PADS 9.5 : Schematic 및 PCB Layout 설계

## 5. 프로젝트 진행하면서 깨달은 점
- 폴링 방식을 넘어 인터럽트 방식을 활용한 데이터 수신 및 처리 방법 학습
- UART, SPI 및 I2C 통신 프로토콜의 원리를 깊이 이해하고 이를 기반으로 라이브러리를 구현
- 베이지안 필터와 칼만 필터에 대한 처음 접하게 되었으며, 이를 통해 실시간 데이터 필터링 및 추정을 수행하는 능력을 향상