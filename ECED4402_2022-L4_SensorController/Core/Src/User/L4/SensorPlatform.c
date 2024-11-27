/*
 * remoteSensingPlatform.c
 *
 *  Created on: Oct. 21, 2022
 *      Author: Andre Hendricks / Dr. JF Bousquet
 */
#include <stdio.h>
#include <User/L3/InfraredSensor.h>
#include <User/L3/UltrasonicSensor.h>
#include "User/L2/Comm_Datalink.h"
#include "User/L4/SensorPlatform.h"
#include "User/util.h"
#include "FreeRTOS.h"
#include "Timers.h"
#include "semphr.h"

static void ResetMessageStruct(struct CommMessage* currentRxMessage){

	static const struct CommMessage EmptyMessage = {0};
	*currentRxMessage = EmptyMessage;
}

/******************************************************************************
This task is created from the main.
It is responsible for managing the messages from the datalink.
It is also responsible for starting the timers for each sensor
******************************************************************************/
void SensorPlatformTask(void *params)
{
    const TickType_t TimerDefaultPeriod = 10000;
    TimerHandle_t TimerID_UltrasonicSensor, TimerID_InfraredSensor;

    // Create timers for ultrasonic and infrared sensors
    TimerID_UltrasonicSensor = xTimerCreate("Ultrasonic Sensor Task", TimerDefaultPeriod, pdTRUE, (void*)1, RunUltrasonicSensor);
    TimerID_InfraredSensor = xTimerCreate("Infrared Sensor Task", TimerDefaultPeriod, pdTRUE, (void*)4, RunIRSensor);

    request_sensor_read();  // requests a usart read (through the callback)

    struct CommMessage currentRxMessage = {0};

    do {

        parse_sensor_message(&currentRxMessage);

        if(currentRxMessage.IsMessageReady == true && currentRxMessage.IsCheckSumValid == true){

            switch(currentRxMessage.SensorID){
                case Controller:
                    if(currentRxMessage.messageId == 0){
                        xTimerStop(TimerID_UltrasonicSensor,  portMAX_DELAY);
                        xTimerStop(TimerID_InfraredSensor,  portMAX_DELAY);
                        send_ack_message(RemoteSensingPlatformReset);
                    }
                    break;
                case Ultrasonic:
                    if(currentRxMessage.messageId == 0){
                        xTimerChangePeriod(TimerID_UltrasonicSensor, currentRxMessage.params, portMAX_DELAY);
                        xTimerStart(TimerID_UltrasonicSensor, portMAX_DELAY);
                        send_ack_message(UltrasonicSensorEnable);
                    }
                    break;
                case Infrared:
                    if(currentRxMessage.messageId == 0){
                        xTimerChangePeriod(TimerID_InfraredSensor, currentRxMessage.params, portMAX_DELAY);
                        xTimerStart(TimerID_InfraredSensor, portMAX_DELAY);
                        send_ack_message(InfraredSensorEnable);
                    }
                    break;
                default:
                    break;
            }
            ResetMessageStruct(&currentRxMessage);
        }
    } while(1);
}
