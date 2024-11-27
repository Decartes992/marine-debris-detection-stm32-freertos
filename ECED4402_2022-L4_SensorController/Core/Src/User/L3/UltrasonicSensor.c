#include <stdlib.h>
#include <User/L3/UltrasonicSensor.h>

#include "User/L2/Comm_Datalink.h"
#include "FreeRTOS.h"
#include "Timers.h"
#include "stm32f4xx_hal.h"

#define TRIG_PIN GPIO_PIN_5
#define ECHO_PIN GPIO_PIN_6
#define TRIG_PORT GPIOA
#define ECHO_PORT GPIOA

void UltrasonicSensor_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Configure TRIG_PIN as output
    GPIO_InitStruct.Pin = TRIG_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(TRIG_PORT, &GPIO_InitStruct);

    // Configure ECHO_PIN as input
    GPIO_InitStruct.Pin = ECHO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(ECHO_PORT, &GPIO_InitStruct);
}

void UltrasonicSensor_SendPulse(void) {
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET);
    HAL_Delay(2); // Ensure pin is low for 2 us
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_SET);
    HAL_Delay(10); // Send 10 us pulse
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET);
}

uint32_t UltrasonicSensor_GetEchoDuration(void) {
    uint32_t startTick = HAL_GetTick();
    while (HAL_GPIO_ReadPin(ECHO_PORT, ECHO_PIN) == GPIO_PIN_RESET) {
        if ((HAL_GetTick() - startTick) > 100) return 0; // Timeout
    }
    startTick = HAL_GetTick();
    while (HAL_GPIO_ReadPin(ECHO_PORT, ECHO_PIN) == GPIO_PIN_SET) {
        if ((HAL_GetTick() - startTick) > 100) return 0; // Timeout
    }
    return HAL_GetTick() - startTick;
}

float UltrasonicSensor_GetDistance(void) {
    UltrasonicSensor_SendPulse();
    uint32_t duration = UltrasonicSensor_GetEchoDuration();
    return (duration * 0.0343) / 2.0;
}

void RunUltrasonicSensor(TimerHandle_t xTimer) {
    float distance = UltrasonicSensor_GetDistance();
    send_sensorData_message(Ultrasonic, (uint16_t)distance);
}
