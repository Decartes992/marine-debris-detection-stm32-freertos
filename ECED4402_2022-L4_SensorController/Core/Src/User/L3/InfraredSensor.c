#include <stdlib.h>
#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "User/L3/InfraredSensor.h"
#include "User/L2/Comm_Datalink.h"
#include "FreeRTOS.h"
#include "timers.h"
#include "User/util.h"  // Add this for print_str

#define IR_PIN GPIO_PIN_7
#define IR_PORT GPIOA

volatile uint8_t irSignalState = 0;
static uint32_t readingCount = 0;  // Counter for readings

void InfraredSensor_Init() {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    char str[50];

    // Configure PA7 as simple GPIO input
    GPIO_InitStruct.Pin = IR_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(IR_PORT, &GPIO_InitStruct);

    sprintf(str, "IR Sensor initialized on PA7\r\n");
    print_str(str);
}

uint8_t InfraredSensor_ReadSignal() {
    uint8_t value = HAL_GPIO_ReadPin(IR_PORT, IR_PIN);
    char str[100];

    // Print raw reading every 1000 readings to avoid flooding
    if (readingCount % 1000 == 0) {
        sprintf(str, "IR Raw Reading [%lu]: %d\r\n", readingCount, value);
        print_str(str);
    }
    readingCount++;

    return value;
}

void RunIRSensor(TimerHandle_t xTimer) {
    

	char str[100];
    uint8_t irValue = InfraredSensor_ReadSignal();

    // Print debug info for each timer callback
    sprintf(str, "IR Timer Callback - Value: %d, Count: %lu\r\n",
            irValue, readingCount);
    print_str(str);

    // Send the sensor reading
    send_sensorData_message(Infrared, (uint16_t)irValue);

    // Update global state if needed
    irSignalState = irValue;

    // Print if state changed
    static uint8_t lastState = 0xFF;  // Initialize to invalid state
    if (irSignalState != lastState) {
        sprintf(str, "IR State Changed: %d -> %d\r\n",
                lastState, irSignalState);
        print_str(str);
        lastState = irSignalState;
    }
    

}
