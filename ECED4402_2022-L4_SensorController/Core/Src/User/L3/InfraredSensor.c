#include <stdlib.h>
#include "stm32f4xx_hal.h"
#include <User/L3/InfraredSensor.h>
#include "FreeRTOS.h"
#include "timers.h"
#include "User/L2/Comm_Datalink.h"

// Define Infrared Receiver GPIO Pin and Port
#define IR_PIN GPIO_PIN_7
#define IR_PORT GPIOA

// Global variable to store the state of the infrared sensor
volatile uint8_t irSignalState = 0;

// Function to initialize the infrared receiver as GPIO input
void InfraredSensor_Init() {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Configure PA7 as GPIO input
    GPIO_InitStruct.Pin = IR_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;  // Set as input mode
    GPIO_InitStruct.Pull = GPIO_NOPULL;      // No pull-up or pull-down
    HAL_GPIO_Init(IR_PORT, &GPIO_InitStruct);
}

// Function to read the infrared receiver signal
uint8_t InfraredSensor_ReadSignal() {
    return HAL_GPIO_ReadPin(IR_PORT, IR_PIN);  // Read the GPIO pin state
}

// FreeRTOS task or periodic callback function for monitoring the sensor
void RunIRSensor(TimerHandle_t xTimer) {
    // Read the signal state from the infrared sensor
    irSignalState = InfraredSensor_ReadSignal();

    // Process the signal state (e.g., send it over UART or handle it in logic)
    if (irSignalState) {
        // Signal detected (HIGH state)
        send_sensorData_message(Infrared, 1);  // Send a HIGH state as data
    } else {
        // No signal detected (LOW state)
        send_sensorData_message(Infrared, 0);  // Send a LOW state as data
    }
}
