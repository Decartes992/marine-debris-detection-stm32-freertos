#include <stdlib.h>
#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "User/L3/InfraredSensor.h"
#include "User/L2/Comm_Datalink.h"
#include "FreeRTOS.h"
#include "timers.h"
#include "User/util.h"

// Pin definitions
#define IR_PIN GPIO_PIN_7
#define IR_PORT GPIOA

// Timer definitions
TIM_HandleTypeDef htim3;
TIM_IC_InitTypeDef sConfig;

// Status variables
static uint32_t lastCaptureValue = 0;
static uint32_t currentCaptureValue = 0;
static uint32_t signalDuration = 0;
static uint32_t readingCount = 0;

// Debris classification thresholds (in microseconds)
#define PLASTIC_MIN_DURATION 2000
#define PLASTIC_MAX_DURATION 3000
#define ORGANIC_MIN_DURATION 1000
#define ORGANIC_MAX_DURATION 1800
#define METAL_MIN_DURATION   500
#define METAL_MAX_DURATION   900

void InfraredSensor_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    char str[100];

    // Enable GPIO clock
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_TIM3_CLK_ENABLE();

    // Configure IR sensor input pin
    GPIO_InitStruct.Pin = IR_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(IR_PORT, &GPIO_InitStruct);

    // Configure Timer3 for input capture
    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 84-1;  // 84MHz/84 = 1MHz timer clock
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 0xFFFF;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_IC_Init(&htim3);

    // Configure Input Capture channel
    sConfig.ICPolarity = TIM_ICPOLARITY_FALLING;
    sConfig.ICSelection = TIM_ICSELECTION_DIRECTTI;
    sConfig.ICPrescaler = TIM_ICPSC_DIV1;
    sConfig.ICFilter = 0x0;
    HAL_TIM_IC_ConfigChannel(&htim3, &sConfig, TIM_CHANNEL_2);

    // Start Input Capture
    HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_2);

    sprintf(str, "IR Sensor initialized on PA7 with Timer3\r\n");
    print_str(str);
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
    char str[100];

    if (htim->Instance == TIM3) {
        lastCaptureValue = currentCaptureValue;
        currentCaptureValue = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);

        // Calculate signal duration
        if (currentCaptureValue > lastCaptureValue) {
            signalDuration = currentCaptureValue - lastCaptureValue;
        } else {
            signalDuration = ((0xFFFF - lastCaptureValue) + currentCaptureValue + 1);
        }

        // Debug: Log raw values and calculated duration
        sprintf(str, "Capture Callback: LastValue=%lu, CurrentValue=%lu, Duration=%lu us\r\n",
                lastCaptureValue, currentCaptureValue, signalDuration);
        print_str(str);
    }
}

uint8_t ClassifyDebris(uint32_t duration) {
    char str[100];
    uint8_t debrisType;

    if (duration >= PLASTIC_MIN_DURATION && duration <= PLASTIC_MAX_DURATION) {
        debrisType = PLASTIC_DEBRIS;
    } else if (duration >= ORGANIC_MIN_DURATION && duration <= ORGANIC_MAX_DURATION) {
        debrisType = ORGANIC_DEBRIS;
    } else if (duration >= METAL_MIN_DURATION && duration <= METAL_MAX_DURATION) {
        debrisType = METAL_DEBRIS;
    } else {
        debrisType = UNKNOWN_DEBRIS;
    }

    // Debug: Log classification
    sprintf(str, "ClassifyDebris: Duration=%lu us, Classified as=%s\r\n",
            duration,
            debrisType == PLASTIC_DEBRIS ? "Plastic" :
            debrisType == ORGANIC_DEBRIS ? "Organic" :
            debrisType == METAL_DEBRIS ? "Metal" : "Unknown");
    print_str(str);

    return debrisType;
}

void RunIRSensor(TimerHandle_t xTimer) {
    char str[100];
    uint8_t debrisType;
    static uint32_t lastReportedType = UNKNOWN_DEBRIS;

    if (signalDuration > 0) {
        debrisType = ClassifyDebris(signalDuration);

        // Only report if classification changes
        if (debrisType != lastReportedType) {
            sprintf(str, "RunIRSensor: Reading %lu: Duration=%lu us, Type=%s\r\n",
                    readingCount,
                    signalDuration,
                    debrisType == PLASTIC_DEBRIS ? "Plastic" :
                    debrisType == ORGANIC_DEBRIS ? "Organic" :
                    debrisType == METAL_DEBRIS ? "Metal" : "Unknown");
            print_str(str);

            send_sensorData_message(Infrared, debrisType);
            lastReportedType = debrisType;
        }

        readingCount++;
    } else {
        // Debug: No signal detected
        sprintf(str, "RunIRSensor: No signal detected (Reading %lu)\r\n", readingCount);
        print_str(str);
    }
}
