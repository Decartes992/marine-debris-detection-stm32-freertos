/*
 * remoteSensingPlatform.c
 *
 *  Created on: Oct. 21, 2022
 *      Author: Andre Hendricks / Dr. JF Bousquet
 */
#include <stdio.h>

#include "User/L2/Comm_Datalink.h"
#include "User/L3/AcousticSensor.h"
#include "User/L3/DepthSensor.h"
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

	while(1) {
		request_sensor_read();
		print_str("NEW DATA: ");
		print_str(rx_buffer_extern);
		print_str("\r\n");
	}






	const TickType_t TimerDefaultPeriod = 1000;
	TimerHandle_t TimerID_AcousticSensor,TimerID_DepthSensor;

	TimerID_DepthSensor = xTimerCreate(
		"Depth Sensor Task",
		TimerDefaultPeriod,		// Period: Needed to be changed based on parameter
		pdTRUE,		// Autoreload: Continue running till deleted or stopped
		(void*)0,
		RunDepthSensor
		);

	TimerID_AcousticSensor = xTimerCreate(
		"Acoustic Sensor Task",
		TimerDefaultPeriod,		// Period: Needed to be changed based on parameter
		pdTRUE,		// Autoreload: Continue running till deleted or stopped
		(void*)1,
		RunAcousticSensor
		);

	request_sensor_read();  // requests a usart read (through the callback)

	struct CommMessage currentRxMessage = {0};

	do {

		parse_sensor_message(&currentRxMessage);

		if(currentRxMessage.IsMessageReady == true && currentRxMessage.IsCheckSumValid == true){

			switch(currentRxMessage.SensorID){
				case Controller:
					switch(currentRxMessage.messageId){
						case 0:
							xTimerStop(TimerID_DepthSensor, portMAX_DELAY);
							xTimerStop(TimerID_AcousticSensor, portMAX_DELAY);
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
				case Depth:
					switch(currentRxMessage.messageId){
						case 0:
							xTimerChangePeriod(TimerID_DepthSensor, currentRxMessage.params, portMAX_DELAY);
							xTimerStart(TimerID_DepthSensor, portMAX_DELAY);
							send_ack_message(DepthSensorEnable);
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

#ifdef oldcodes


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
	const TickType_t TimerDefaultPeriod = 1000;
	TimerHandle_t TimerID_AcousticSensor,TimerID_DepthSensor;

	TimerID_DepthSensor = xTimerCreate(
		"Depth Sensor Task",
		TimerDefaultPeriod,		// Period: Needed to be changed based on parameter
		pdTRUE,		// Autoreload: Continue running till deleted or stopped
		(void*)0,
		RunDepthSensor
		);

	TimerID_AcousticSensor = xTimerCreate(
		"Acoustic Sensor Task",
		TimerDefaultPeriod,		// Period: Needed to be changed based on parameter
		pdTRUE,		// Autoreload: Continue running till deleted or stopped
		(void*)1,
		RunAcousticSensor
		);

	request_sensor_read();  // requests a usart read (through the callback)

	/*
	 *
	struct CommMessage
{
	enum SensorId_t SensorID;
	uint8_t messageId;
	uint16_t params;
	uint8_t checksum;
	bool IsCheckSumValid;
	bool IsMessageReady; // This flag indicates if a message has been decoded by the datalink.
};
	 *
	 */

	struct CommMessage currentRxMessage = {0};

	do {

		parse_sensor_message(&currentRxMessage);

		if(currentRxMessage.IsMessageReady == true && currentRxMessage.IsCheckSumValid == true){

			switch(currentRxMessage.SensorID){
				case Controller:
					switch(currentRxMessage.messageId){
						case 0: // reset sensors
							print_str("reset sensors \n\r");
							xTimerStop(TimerID_DepthSensor, portMAX_DELAY);
							xTimerStop(TimerID_AcousticSensor, portMAX_DELAY);
							send_ack_message(RemoteSensingPlatformReset);
							break;
						case 1: // sensor reset ack
							print_str("reset ack send \n\r");
							xTimerStart(TimerID_DepthSensor, portMAX_DELAY);
							xTimerStart(TimerID_AcousticSensor, portMAX_DELAY);
							send_ack_message(RemoteSensingPlatformReset);
							break;
						case 3: //Do Nothing, no case 3
							break;
						}
					break;
				case Acoustic:
					switch(currentRxMessage.messageId){
						case 0: // Enable sensor
							print_str("Acoustic enable sensor \n\r");
							xTimerChangePeriod(TimerID_AcousticSensor, currentRxMessage.params, portMAX_DELAY);
							xTimerStart(TimerID_AcousticSensor, portMAX_DELAY);
							send_sensorEnable_message(AcousticSensorEnable,currentRxMessage.params);
							break;
						case 1: // Acoustic sensor enable ack
							print_str("Acoustic ack sensor \n\r");
							xTimerStart(TimerID_AcousticSensor, portMAX_DELAY);
							send_ack_message(AcousticSensorEnable);
							break;
						case 3: // Acoustic data
							print_str("Acoustic sensor Data \n\r");
							send_sensorData_message(Acoustic, currentRxMessage.params);
							break;
					}
					break;
				case Depth:
					switch(currentRxMessage.messageId){
						case 0: // Enable sensor
							print_str("Depth Enable sensor \n\r");
							xTimerChangePeriod(TimerID_DepthSensor, currentRxMessage.params, portMAX_DELAY);
							xTimerStart(TimerID_DepthSensor, portMAX_DELAY);
							send_sensorEnable_message(DepthSensorEnable, currentRxMessage.params);
							break;
						case 1: // Enable Ack
							print_str("Depth Ack sensor \n\r");
							xTimerStart(TimerID_DepthSensor, portMAX_DELAY);
							send_ack_message(DepthSensorEnable);
							break;
						case 3: // Depth Data
							print_str("Depth Data \n\r");
							send_sensorData_message(Depth, currentRxMessage.params);
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
#endif