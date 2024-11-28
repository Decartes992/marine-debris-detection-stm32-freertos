#ifndef INFRAREDSENSOR_H_
#define INFRAREDSENSOR_H_

#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "timers.h"

// Debris type definitions
#define UNKNOWN_DEBRIS 0
#define PLASTIC_DEBRIS 1
#define ORGANIC_DEBRIS 2
#define METAL_DEBRIS   3

// Function prototypes
void InfraredSensor_Init(void);
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim);
uint8_t ClassifyDebris(uint32_t duration);
void RunIRSensor(TimerHandle_t xTimer);

// External declarations for timer handle if needed in other files
extern TIM_HandleTypeDef htim3;

#endif /* INFRAREDSENSOR_H_ */
