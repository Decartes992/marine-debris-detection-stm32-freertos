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

#include "User/L3/AcousticSensor.h"
#include "User/L3/CorrosionSensor.h"
#include "User/L4/SensorPlatform.h"

#include "User/util.h"

//Required FreeRTOS header files
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
	TimerHandle_t TimerID_AcousticSensor, TimerID_UltrasonicSensor, TimerID_FlowRateSensor, TimerID_CorrosionSensor, TimerID_InfraredSensor;

	TimerID_AcousticSensor = xTimerCreate("Acoustic Sensor Task", TimerDefaultPeriod, pdTRUE, (void*)0, RunAcousticSensor);
	TimerID_UltrasonicSensor = xTimerCreate("Ultrasonic Sensor Task", TimerDefaultPeriod, pdTRUE, (void*)1, RunUltrasonicSensor);
	TimerID_FlowRateSensor = xTimerCreate("Flow Rate Sensor Task", TimerDefaultPeriod, pdTRUE, (void*)2, RunFlowRateSensor);
	//TimerID_CorrosionSensor = xTimerCreate("Corrosion Sensor Task", TimerDefaultPeriod, pdTRUE, (void*)3, RunCorrosionSensor);
	TimerID_InfraredSensor = xTimerCreate("Infrared Sensor Task", TimerDefaultPeriod, pdTRUE, (void*)4, RunIRSensor);

	request_sensor_read();  // requests a usart read (through the callback)

	struct CommMessage currentRxMessage = {0};

	do {

		parse_sensor_message(&currentRxMessage);

		if(currentRxMessage.IsMessageReady == true && currentRxMessage.IsCheckSumValid == true){

			switch(currentRxMessage.SensorID){
				case Controller:
					switch(currentRxMessage.messageId){
						case 0:
							xTimerStop(TimerID_AcousticSensor,  portMAX_DELAY);
							xTimerStop(TimerID_UltrasonicSensor,  portMAX_DELAY);
							xTimerStop(TimerID_FlowRateSensor,  portMAX_DELAY);
							//xTimerStop(TimerID_CorrosionSensor, portMAX_DELAY);
							send_ack_message(RemoteSensingPlatformReset);
							break;
						case 1: //Do Nothing
							break;
						case 3: //Do Nothing
							break;
						}
					break;
				case Acoustic:
					switch(currentRxMessage.messageId){
						case 0:
							xTimerChangePeriod(TimerID_AcousticSensor, currentRxMessage.params, portMAX_DELAY);
							xTimerStart(TimerID_AcousticSensor, portMAX_DELAY);
							send_ack_message(AcousticSensorEnable);
							break;
						case 1: //Do Nothing
							break;
						case 3: //Do Nothing
							break;
					}
					break;
					case Ultrasonic:
						switch(currentRxMessage.messageId){
							case 0:
								xTimerChangePeriod(TimerID_UltrasonicSensor, currentRxMessage.params, portMAX_DELAY);
								xTimerStart(TimerID_UltrasonicSensor, portMAX_DELAY);
								send_ack_message(UltrasonicSensorEnable);
								break;
							case 1: //Do Nothing
								break;
							case 3: //Do Nothing
								break;
						}
						break;
					case FlowRate:
						switch(currentRxMessage.messageId){
							case 0:
								xTimerChangePeriod(TimerID_FlowRateSensor, currentRxMessage.params, portMAX_DELAY);
								xTimerStart(TimerID_FlowRateSensor, portMAX_DELAY);
								send_ack_message(FlowRateSensorEnable);
								break;
							case 1: //Do Nothing
								break;
							case 3: //Do Nothing
								break;
						}
						break;
					/*case Corrosion:
						switch(currentRxMessage.messageId){
							case 0:
								xTimerChangePeriod(TimerID_CorrosionSensor, currentRxMessage.params, portMAX_DELAY);
								xTimerStart(TimerID_CorrosionSensor, portMAX_DELAY);
								send_ack_message(CorrosionSensorEnable);
								break;
							case 1: //Do Nothing
								break;
							case 3: //Do Nothing
								break;
						}
						break;*/
					case Infrared:
						switch(currentRxMessage.messageId){
							case 0:
								xTimerChangePeriod(TimerID_InfraredSensor, currentRxMessage.params, portMAX_DELAY);
								xTimerStart(TimerID_InfraredSensor, portMAX_DELAY);
								send_ack_message(InfraredSensorEnable);
								break;
							case 1: //Do Nothing
								break;
							case 3: //Do Nothing
								break;
						}
						break;
					default://Should not get here
						break;
			}
			ResetMessageStruct(&currentRxMessage);
		}
	} while(1);
}
