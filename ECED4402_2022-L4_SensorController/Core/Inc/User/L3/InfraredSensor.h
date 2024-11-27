#ifndef INC_USER_L3_INFRAREDSENSOR_H_
#define INC_USER_L3_INFRAREDSENSOR_H_

#include "FreeRTOS.h"
#include "timers.h"

void InfraredSensor_Init(void);
int ReadInfraredSensor(void);
void DecodeIRSignal(uint32_t pulseWidth);
void ProcessIRData(uint32_t data);
void RunIRSensor(TimerHandle_t xTimer);

#endif /* INC_USER_L3_INFRAREDSENSOR_H_ */
