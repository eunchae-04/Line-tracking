#include "tm4c123gh6pm.h"
#include "ir.h"

// 내부에서만 사용할 함수 선언 (main에서는 몰라도 됨)
void PortE_Input_Init(void);
void PortF_Output_Init(void);

// 1. 전체 시스템 초기화 (IR + LED)
void System_Init(void) {
    PortE_Input_Init(); // IR 초기화
    PortF_Output_Init(); // LED 초기화
}

// 2. IR 센서 값 읽기
unsigned long IR_Read(void) {
    // PE1, PE2 값만 마스킹해서 리턴 (0x02, 0x04, 0x06 등)
    return (GPIO_PORTE_DATA_R & 0x06);
}

// 3. 빨간색 LED (PF1) 제어
void LED_Red_Set(int state) {
    if(state) {
        GPIO_PORTF_DATA_R |= 0x02; // 켜기
    } else {
        GPIO_PORTF_DATA_R &= ~0x02; // 끄기
    }
}

// 4. 파란색 LED (PF2) 제어
void LED_Blue_Set(int state) {
    if(state) {
        GPIO_PORTF_DATA_R |= 0x04; // 켜기
    } else {
        GPIO_PORTF_DATA_R &= ~0x04; // 끄기
    }
}

// --- 아래는 기존의 하드웨어 초기화 코드 (내부 호출용) ---

void PortE_Input_Init(void) {
    volatile unsigned long delay;
    SYSCTL_RCGCGPIO_R |= 0x10;      // Port E 클럭 활성화
    delay = SYSCTL_RCGCGPIO_R;      
    GPIO_PORTE_AMSEL_R &= ~0x06;    // PE1, PE2 아날로그 끔
    GPIO_PORTE_PCTL_R &= ~0x00000FF0; 
    GPIO_PORTE_DIR_R &= ~0x06;      // 입력 설정
    GPIO_PORTE_AFSEL_R &= ~0x06;    
    GPIO_PORTE_DEN_R |= 0x06;       // 디지털 활성화
}

void PortF_Output_Init(void) {
    volatile unsigned long delay;
    SYSCTL_RCGCGPIO_R |= 0x20;      // Port F 클럭 활성화
    delay = SYSCTL_RCGCGPIO_R;      
    GPIO_PORTF_LOCK_R = 0x4C4F434B; 
    GPIO_PORTF_CR_R = 0x1F;         
    GPIO_PORTF_AMSEL_R = 0x00;      
    GPIO_PORTF_PCTL_R = 0x00000000; 
    GPIO_PORTF_DIR_R |= 0x06;       // 출력 설정 (PF1, PF2)
    GPIO_PORTF_AFSEL_R = 0x00;      
    GPIO_PORTF_DEN_R |= 0x06;       // 디지털 활성화
    GPIO_PORTF_DATA_R &= ~0x06;     // LED 끄고 시작
}
