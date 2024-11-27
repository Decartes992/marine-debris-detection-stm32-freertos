/*
 * SensorController.h
 *
 *  Created on: Oct 24, 2022
 *      Author: kadh1
 */

#ifndef INC_USER_L4_SENSORCONTROLLER_H_
#define INC_USER_L4_SENSORCONTROLLER_H_

struct SensorStates {
    bool IsUltrasonicAck;
    bool IsInfraredAck;

    uint16_t UltrasonicData;
    uint16_t InfraredData;
};

void HostPC_RX_Task();
void SensorPlatform_RX_Task();
void SensorControllerTask(void *params);

char* analyzeUltrasonicValue(int ultrasonicValue);
char* analyzeInfraredValue(int infraredValue);

#endif /* INC_USER_L4_SENSORCONTROLLER_H_ */
