#include <stdlib.h>
#include <stdio.h>
#include <User/L3/UltrasonicSensor.h>
#include "User/L2/Comm_Datalink.h"
#include "FreeRTOS.h"
#include "Timers.h"
#include "stm32f4xx_hal.h"
#include "User/util.h"

#define TRIG_PIN GPIO_PIN_1
#define TRIG_PORT GPIOA
#define ECHO_PIN GPIO_PIN_6
#define ECHO_PORT GPIOA
#define DISTANCE_LIMIT_CM 400  // Maximum valid distance in cm
#define SPEED_OF_SOUND 340    // Speed of sound in m/s

static uint32_t readingCount = 0;
static uint32_t lastValidDistance = 0;

void UltrasonicSensor_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    char str[100];

    // Configure TRIG_PIN as output
    GPIO_InitStruct.Pin = TRIG_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;  // Changed to HIGH for faster response
    HAL_GPIO_Init(TRIG_PORT, &GPIO_InitStruct);

    // Configure ECHO_PIN as input
    GPIO_InitStruct.Pin = ECHO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(ECHO_PORT, &GPIO_InitStruct);

    // Ensure trigger starts low
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET);

    sprintf(str, "Ultrasonic Sensor initialized - Trigger:PA1, Echo:PA6\r\n");
    print_str(str);
}

static void SendTriggerPulse(void) {
    // Generate 10µs trigger pulse
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_SET);
    vTaskDelay(pdMS_TO_TICKS(1));  // 1ms is more than enough for 10µs
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET);
}

uint16_t UltrasonicSensor_GetDistance(void) {
    uint32_t pulseDuration;
    uint32_t startTime;
    uint16_t distance;
    char str[100];
    const uint32_t TIMEOUT = 100;  // 100ms timeout

    // Send trigger pulse
    SendTriggerPulse();

    // Wait for echo to start (rising edge)
    startTime = HAL_GetTick();
    while (HAL_GPIO_ReadPin(ECHO_PORT, ECHO_PIN) == GPIO_PIN_RESET) {
        if ((HAL_GetTick() - startTime) > TIMEOUT) {
            sprintf(str, "Echo start timeout [%lu]\r\n", readingCount);
            print_str(str);
            return lastValidDistance;
        }
    }

    // Measure pulse width
    startTime = HAL_GetTick();
    while (HAL_GPIO_ReadPin(ECHO_PORT, ECHO_PIN) == GPIO_PIN_SET) {
        if ((HAL_GetTick() - startTime) > TIMEOUT) {
            sprintf(str, "Echo end timeout [%lu]\r\n", readingCount);
            print_str(str);
            return lastValidDistance;
        }
    }
    pulseDuration = HAL_GetTick() - startTime;

    // Calculate distance in cm: distance = (time * speed of sound) / 2
    // Using integer math: distance = (duration_ms * 34000) / (2 * 1000)
    distance = (uint16_t)((pulseDuration * 34) / 2);

    // Validate reading
    if (distance <= DISTANCE_LIMIT_CM) {
        lastValidDistance = distance;
        sprintf(str, "Valid distance: %u cm [%lu]\r\n", distance, readingCount);
    } else {
        distance = lastValidDistance;
        sprintf(str, "Invalid reading, using last valid distance: %u cm [%lu]\r\n",
                distance, readingCount);
    }
    print_str(str);

    return distance;
}

void RunUltrasonicSensor(TimerHandle_t xTimer) {
    char str[100];
    static uint16_t lastReportedDistance = 0;

    sprintf(str, "Reading ultrasonic sensor [%lu]\r\n", readingCount);
    print_str(str);

    uint16_t currentDistance = UltrasonicSensor_GetDistance();

    // Report changes greater than 5cm
    if (abs(currentDistance - lastReportedDistance) > 5) {
        sprintf(str, "Significant distance change: %u -> %u cm\r\n",
                lastReportedDistance, currentDistance);
        print_str(str);
        lastReportedDistance = currentDistance;
    }

    send_sensorData_message(Ultrasonic, currentDistance);
    readingCount++;
}
