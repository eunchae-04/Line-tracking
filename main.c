#include "tm4c123gh6pm.h"
#include <stdint.h>
#include <stdlib.h> 

// 튜닝 파라미터
#define IR_THRESHOLD     500      
#define IR_WHITE_LIMIT   1500     
#define PWM_PERIOD       1000 

// 속도 설정
#define SPEED_BASE       650     
#define SPEED_TURN_BASE  300     // 코너링 진입 속도
#define SPEED_MAX        950     // 95% 제한
#define SPEED_MIN        50      // 5% 제한
#define SPEED_SEARCH     600     // 탐색 속도
#define RIGHT_MOTOR_GAIN 1.15    // 하드웨어 보정 게인

// 이탈 방지 & 방향 기억
#define LOST_LINE_DELAY  5       // 5ms 이상 감지 안 되면 이탈로 간주
#define DIR_THRESHOLD    200     // 방향 기억 갱신 임계값

// 제어 상수
#define K_P              16
#define K_D              30      

// 전역 변수
volatile uint32_t SensorValues[3] = {0, 0, 0}; 
volatile long LastError = 0; 
volatile int LostLineCounter = 0;
volatile int LastValidDirection = 0; 
volatile int g_TickSemaphore; // 세마포어 변수

// 세마포어 초기화
void OS_Semaphore_Init(volatile int *sema, int initialValue) {
    *sema = initialValue;
}

// [Signal / V연산] 자원을 놓아줌 (ISR에서 호출)
void OS_Signal(volatile int *sema) {
    (*sema)++; // 세마포어 카운트 증가
}

// [Wait / P연산] 자원을 획득할 때까지 대기 (Main에서 호출)
void OS_Wait(volatile int *sema) {
    // 세마포어 값이 0보다 작거나 같으면 무한 대기
    while((*sema) <= 0) {
        // 대기
    }
    (*sema)--; // 자원 획득 후 카운트 감소
}

// 함수 선언
void System_Init(void);
void ADC_Input_Init(void);
void PortF_Init(void);
void PortA_Output_Init(void);
void Hardware_PWM_Init(void);
void SysTick_Init(void);
uint32_t ADC_Read_Average(uint32_t channelNum); 
void EnableInterrupts(void); 

void Set_Motors(long speed_L, long speed_R);
void Set_LED_Status(long error, int isLost);

int main(void) {
    long numerator = 0, denominator = 0;
    long error = 0, derivative = 0, correction = 0;
    long speed_base = SPEED_BASE; 
    int is_line_lost = 0; 

    System_Init();
    GPIO_PORTA_DATA_R |= 0x14; 
    
    // 세마포어 초기화: 0으로 시작
    OS_Semaphore_Init(&g_TickSemaphore, 0);

    EnableInterrupts(); 

    while(1) {
        // [Main Loop] 데이터가 준비될 때까지 대기
        OS_Wait(&g_TickSemaphore); 


        // 1. 에러 계산 및 라인 상태 판단
        numerator   = ((long)SensorValues[0] * 1000) - ((long)SensorValues[2] * 1000);
        denominator = (long)SensorValues[0] + (long)SensorValues[1] + (long)SensorValues[2];
        
        is_line_lost = (denominator < IR_THRESHOLD || 
                       (SensorValues[0] < IR_WHITE_LIMIT && SensorValues[1] < IR_WHITE_LIMIT && SensorValues[2] < IR_WHITE_LIMIT));

        // 2. 주행 제어 (알고리즘)
        if (is_line_lost) {
            LostLineCounter++;
            
            if (LostLineCounter > LOST_LINE_DELAY) {
                if (LastValidDirection > 0) { 
                    Set_Motors(SPEED_SEARCH, 0); 
                } 
                else { 
                    Set_Motors(0, SPEED_SEARCH); 
                }
                Set_LED_Status(0, 1); 
            }
        } 
        else {
            LostLineCounter = 0;
            
            error = numerator / denominator;
            
            if (error > DIR_THRESHOLD)       LastValidDirection = 1;  
            else if (error < -DIR_THRESHOLD) LastValidDirection = -1; 
            
            derivative = error - LastError;
            correction = (error * K_P) + (derivative * K_D);
            LastError = error;

            if (abs(error) > 300) speed_base = SPEED_TURN_BASE;
            else                  speed_base = SPEED_BASE;

            Set_Motors(speed_base + correction, speed_base - correction);
            Set_LED_Status(error, 0);
        }
    }
}

// SysTick 핸들러
void SysTick_Handler(void) {
    // 1. 센서 읽기
    SensorValues[0] = ADC_Read_Average(0); // PE3 (우)
    SensorValues[1] = ADC_Read_Average(1); // PE2 (중)
    SensorValues[2] = ADC_Read_Average(2); // PE1 (좌)

    // 2. 데이터 준비 완료 신호 전송
    OS_Signal(&g_TickSemaphore);
}

// 모터 설정 함수
void Set_Motors(long speed_L, long speed_R) {
    speed_R = (long)(speed_R * RIGHT_MOTOR_GAIN);
    if (speed_L > SPEED_MAX) speed_L = SPEED_MAX;
    if (speed_L < SPEED_MIN) speed_L = SPEED_MIN;
    if (speed_R > SPEED_MAX) speed_R = SPEED_MAX;
    if (speed_R < SPEED_MIN) speed_R = SPEED_MIN;
    PWM0_0_CMPA_R = (unsigned long)speed_L;
    PWM0_0_CMPB_R = (unsigned long)speed_R;
}

// LED 상태 표시 함수
void Set_LED_Status(long error, int isLost) {
    if (isLost) {
        GPIO_PORTF_DATA_R = 0x0E; 
    } else {
        if (error > 200)       GPIO_PORTF_DATA_R = 0x08; 
        else if (error < -200) GPIO_PORTF_DATA_R = 0x02; 
        else                   GPIO_PORTF_DATA_R = 0x04; 
    }
}

// 초기화 함수들
uint32_t ADC_Read_Average(uint32_t channelNum) {
    uint32_t sum = 0;
    int i;
    for(i = 0; i < 4; i++) {
        ADC0_ACTSS_R &= ~0x08; 
        ADC0_SSMUX3_R = channelNum; 
        ADC0_ACTSS_R |= 0x08;
        ADC0_PSSI_R = 0x08;
        while((ADC0_RIS_R & 0x08) == 0); 
        sum += (ADC0_SSFIFO3_R & 0xFFF);
        ADC0_ISC_R = 0x08;
    }
    return sum / 4;
}
void System_Init(void) {
    PortA_Output_Init(); 
    ADC_Input_Init();        
    PortF_Init();          
    Hardware_PWM_Init(); 
    SysTick_Init();        
}
void ADC_Input_Init(void) {
    volatile uint32_t delay;
    SYSCTL_RCGCADC_R |= 0x01;        
    SYSCTL_RCGCGPIO_R |= 0x10;        
    delay = SYSCTL_RCGCGPIO_R;        
    GPIO_PORTE_DIR_R &= ~0x0E;        
    GPIO_PORTE_AFSEL_R |= 0x0E;       
    GPIO_PORTE_DEN_R &= ~0x0E;        
    GPIO_PORTE_AMSEL_R |= 0x0E;       
    ADC0_ACTSS_R &= ~0x08;            
    ADC0_EMUX_R &= ~0xF000;           
    ADC0_SSCTL3_R = 0x06;            
    ADC0_SAC_R = 0x04;                
    ADC0_ACTSS_R |= 0x08;             
}
void Hardware_PWM_Init(void) {
    volatile unsigned long delay;
    SYSCTL_RCGCPWM_R |= 0x01;        
    SYSCTL_RCGCGPIO_R |= 0x02;        
    delay = SYSCTL_RCGCGPIO_R;
    GPIO_PORTB_AFSEL_R |= 0xC0;       
    GPIO_PORTB_PCTL_R &= ~0xFF000000;
    GPIO_PORTB_PCTL_R |= 0x44000000;
    GPIO_PORTB_DEN_R |= 0xC0;         
    SYSCTL_RCC_R |= SYSCTL_RCC_USEPWMDIV; 
    SYSCTL_RCC_R = (SYSCTL_RCC_R & ~SYSCTL_RCC_PWMDIV_M) | SYSCTL_RCC_PWMDIV_16; 
    PWM0_0_CTL_R = 0;                 
    PWM0_0_GENA_R = 0x000000C8;       
    PWM0_0_GENB_R = 0x00000C08;       
    PWM0_0_LOAD_R = PWM_PERIOD - 1; 
    PWM0_0_CMPA_R = 0;                
    PWM0_0_CMPB_R = 0;                
    PWM0_0_CTL_R |= 0x00000001;       
    PWM0_ENABLE_R |= 0x03;            
}
void SysTick_Init(void) {
    NVIC_ST_CTRL_R = 0;              
    NVIC_ST_RELOAD_R = 16000 - 1;    
    NVIC_ST_CURRENT_R = 0;           
    NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R & 0x00FFFFFF) | 0x40000000; 
    NVIC_ST_CTRL_R = 0x07;           
}
void PortA_Output_Init(void) {
    volatile unsigned long delay;
    SYSCTL_RCGCGPIO_R |= 0x01;        
    delay = SYSCTL_RCGCGPIO_R;
    GPIO_PORTA_AMSEL_R &= ~0x3C;      
    GPIO_PORTA_PCTL_R &= ~0x00FFFF00; 
    GPIO_PORTA_DIR_R |= 0x3C;         
    GPIO_PORTA_AFSEL_R &= ~0x3C;      
    GPIO_PORTA_DEN_R |= 0x3C;         
    GPIO_PORTA_DATA_R &= ~0x3C;       
}
void PortF_Init(void) {
    volatile unsigned long delay;
    SYSCTL_RCGCGPIO_R |= 0x20;        
    delay = SYSCTL_RCGCGPIO_R;
    GPIO_PORTF_LOCK_R = 0x4C4F434B; 
    GPIO_PORTF_CR_R |= 0x11;          
    GPIO_PORTF_AMSEL_R = 0x00;        
    GPIO_PORTF_PCTL_R = 0x00000000; 
    GPIO_PORTF_DIR_R |= 0x06;         
    GPIO_PORTF_DIR_R &= ~0x11;        
    GPIO_PORTF_AFSEL_R = 0x00;        
    GPIO_PORTF_PUR_R |= 0x11;         
    GPIO_PORTF_DEN_R |= 0x1F;         
}
