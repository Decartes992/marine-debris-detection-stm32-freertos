#ifndef INC_USER_L3_ULTRASONICSENSOR_H_
#define INC_USER_L3_ULTRASONICSENSOR_H_

#include "FreeRTOS.h"
#include "timers.h"

void RunUltrasonicSensor(TimerHandle_t xTimer); // Default 1000 ms

#endif /* INC_USER_L3_ULTRASONICSENSOR_H_ */
