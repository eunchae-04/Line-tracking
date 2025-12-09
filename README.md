# 🏎️ TM4C123G Line Tracer

**TM4C123G LaunchPad (EK-TM4C123GXL)**를 이용한 자율 주행 라인 트레이서입니다.
단순 On/Off 방식이 아닌 **PD 제어(비례-미분)**와 **1kHz 정주기 제어 시스템**을 도입하여 안정적인 조향과 빠른 주행 성능을 구현했습니다.

## 🗂️ Project Info

| Category | Details |
| --- | --- |
| **개발 기간** | 2025.11.13 ~ 2025.12.10 (4주) |
| **개발 인원** | 2명 |
| **주요 성과** | 1kHz 정주기 제어 구현, PD 제어 최적화, 복합 트랙 완주 |
| **진행 과정** | Delay 기반 기초 구현 → **SysTick 1kHz 동기화** → PD 튜닝 → 주행 테스트 |

---

## 🚀 Key Features

### 1. Deterministic Real-time Control (정주기 제어)
- **SysTick Timer**를 활용한 정확히 **1ms(1kHz)** 주기의 제어 루프 구축
- 기존 Delay 방식의 불규칙한 주기(Jitter)를 제거하여 미분(D) 제어의 수학적 정확도 확보

### 2. Advanced Navigation Algorithm
- **PD Control:** $K_p$, $K_d$ 튜닝을 통해 트랙의 곡률 변화에 부드럽게 반응하며 진동(Oscillation) 억제
- **Weighted Average Sensing:** 3채널 ADC 입력값을 0/1이 아닌 가중 평균으로 처리하여 정밀한 에러($Error$) 산출

### 3. Robust Exception Handling (이탈 방지)
- **Direction Memory:** 라인 소실(Line Lost) 시, 마지막 회전 방향을 기억하여 즉시 **Spin Turn**으로 복귀
- **Safety Design:** 모터 드라이버 보호를 위해 PWM Duty Cycle을 **5%~95%** 범위로 제한(Clamping)

## 🛠️ Hardware Specification

| Component | Model / Description |
| :--- | :--- |
| **MCU** | TI Tiva C Series TM4C123G LaunchPad (EK-TM4C123GXL) |
| **Sensor** | 3-Channel IR Sensor Module (TCRT5000 기반) |
| **Motor Driver** | L298N DC Motor Driver |
| **Actuator** | DC Geared Motor × 2 |
| **Power** | 6V Battery Pack (AA × 4) |

## 🔌 Pin Map

| Function | Pin Name | Description |
| :--- | :--- | :--- |
| **IR Sensor (Input)** | PE3, PE2, PE1 | Right / Center / Left Analog Input (ADC) |
| **PWM (Speed)** | PB6, PB7 | Left / Right Motor PWM Output |
| **Dir (Left)** | PA2, PA3 | Left Motor Direction Control |
| **Dir (Right)** | PA4, PA5 | Right Motor Direction Control |

## 💻 Software Architecture

### Architecture Overview
- **Language:** C (C99 Standard)
- **IDE:** Keil µVision 5
- **Structure:**
  - **Foreground (Main Loop):** 센서 데이터 수집, PD 알고리즘 연산, 모터 제어
  - **Background (ISR):** SysTick Handler를 통한 1ms 타이밍 플래그 생성

### Control Logic Snippet
```c
// 1ms마다 정확하게 실행되는 메인 루프 제어
if (g_TickFlag == 1) {
    g_TickFlag = 0; // 플래그 초기화

    // 1. Sensor Reading (Weighted Average)
    // 2. PD Control: u = (Kp * error) + (Kd * derivative)
    // 3. Motor PWM Update
}
