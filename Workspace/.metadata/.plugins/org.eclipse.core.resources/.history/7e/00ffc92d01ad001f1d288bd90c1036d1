#ifndef INC_USER_L3_ULTRASONICSENSOR_H_
#define INC_USER_L3_ULTRASONICSENSOR_H_

#include "FreeRTOS.h"
#include "timers.h"

void UltrasonicSensor_Init(void);
void UltrasonicSensor_SendPulse(void);
uint32_t UltrasonicSensor_GetEchoDuration(void);
float UltrasonicSensor_GetDistance(void);
void RunUltrasonicSensor(TimerHandle_t xTimer); // Default 1000 ms

#endif /* INC_USER_L3_ULTRASONICSENSOR_H_ */
