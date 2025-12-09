#ifndef IR_H
#define IR_H

// 초기화 함수 (IR과 LED 설정을 모두 포함)
void System_Init(void);

// 센서 값 읽기 함수
unsigned long IR_Read(void);

// LED 제어 함수
void LED_Red_Set(int state);  // 1: 켜기, 0: 끄기
void LED_Blue_Set(int state); // 1: 켜기, 0: 끄기

#endif