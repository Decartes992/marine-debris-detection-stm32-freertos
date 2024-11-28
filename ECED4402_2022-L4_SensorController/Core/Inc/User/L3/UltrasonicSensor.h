#ifndef ULTRASONICSENSOR_H_
#define ULTRASONICSENSOR_H_

#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "timers.h"

// Function prototypes
void UltrasonicSensor_Init(void);
uint16_t UltrasonicSensor_GetDistance(void);
void RunUltrasonicSensor(TimerHandle_t xTimer);

#endif /* ULTRASONICSENSOR_H_ */
