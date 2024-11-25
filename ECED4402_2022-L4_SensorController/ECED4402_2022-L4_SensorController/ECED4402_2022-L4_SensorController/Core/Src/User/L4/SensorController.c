/*
 * SensorController.c
 *
 *  Created on: Oct 24, 2022
 *      Author: kadh1
 */


#include <stdio.h>

#include "main.h"
#include "User/L2/Comm_Datalink.h"
#include "User/L3/AcousticSensor.h"
#include "User/L3/DepthSensor.h"
#include "User/L4/SensorPlatform.h"
#include "User/L4/SensorController.h"
#include "User/util.h"

#include "User/L1/USART_Driver.h"

//Required FreeRTOS header files
#include "FreeRTOS.h"
#include "Timers.h"
#include "semphr.h"

char printstring[50];

char* sensorNames[] = {"Acoustic", "Depth"};

SemaphoreHandle_t sensorMUTEX, pcMUTEX;

QueueHandle_t Queue_Sensor_Data;
QueueHandle_t Queue_HostPC_Data;


static void ResetMessageStruct(struct CommMessage* currentRxMessage){

	static const struct CommMessage EmptyMessage = {0};
	*currentRxMessage = EmptyMessage;
}

/******************************************************************************
This task is created from the main.
******************************************************************************/
void SensorControllerTask(void *params)
{
	do {
		print_str("Controllertsk \r\n");
		vTaskDelay(1000 / portTICK_RATE_MS);

		enum HostPCCommands pcMSG;

		// read PC queue data
		if (uxQueueMessagesWaiting(Queue_HostPC_Data) != 0) {
			xSemaphoreTake(pcMUTEX, portMAX_DELAY);
			xQueueReceive(Queue_HostPC_Data, &pcMSG, portMAX_DELAY);
			xSemaphoreGive(pcMUTEX);

			print_str("Got PC message \r\n");
			vTaskDelay(200 / portTICK_RATE_MS);

			// if start, then start sensors
			if (pcMSG == PC_Command_START) {

				// send start sensors over usart 6
				send_sensorEnable_message(Acoustic, 5);
				send_sensorEnable_message(Depth, 2);
				print_str("Enabled sensors \r\n");

				while (pcMSG != PC_Command_RESET) {

					if (uxQueueMessagesWaiting(Queue_HostPC_Data) != 0) {
						xSemaphoreTake(pcMUTEX, portMAX_DELAY);
						xQueueReceive(Queue_HostPC_Data, &pcMSG, portMAX_DELAY);
						xSemaphoreGive(pcMUTEX);
						print_str("checked PC messages \r\n");
					}

					struct CommMessage SenMSG;

					if (uxQueueMessagesWaiting(Queue_Sensor_Data) != 0) {
						// gather data and print to terminal
						ResetMessageStruct(&SenMSG);

						xSemaphoreTake(sensorMUTEX, portMAX_DELAY);
						xQueueReceive(Queue_Sensor_Data, &SenMSG, portMAX_DELAY);
						xSemaphoreGive(sensorMUTEX);
						print_str("Got   data \r\n");

						enum SensorId_t sensor = SenMSG.SensorID;
						uint16_t data = SenMSG.params;

						sprintf(printstring, "%s %u \r\n", sensorNames[sensor - 2], data);
						print_str(printstring);

					}

					print_str("WAITING \r\n");
					vTaskDelay(1000 / portTICK_RATE_MS);

				}

			}

			send_sensorReset_message();

			// if reset then stop sensors
				// send reset sensors over ussart 6
			//
		}

	} while(1);
}

/*
 * This task reads the queue of characters from the Sensor Platform when available
 * It then sends the processed data to the Sensor Controller Task
 */
void SensorPlatform_RX_Task(){
	struct CommMessage currentRxMessage = {0};

	sensorMUTEX = xSemaphoreCreateMutex();

	Queue_Sensor_Data = xQueueCreate(80, sizeof(struct CommMessage));

	request_sensor_read();  // requests a usart read (through the callback)

	while(1){
		parse_sensor_message(&currentRxMessage);

		if(currentRxMessage.IsMessageReady == true && currentRxMessage.IsCheckSumValid == true){
			print_str("Got sensor data from buffer\r\n");
			xSemaphoreTake(sensorMUTEX, portMAX_DELAY);
			xQueueSendToBack(Queue_Sensor_Data, &currentRxMessage, 0);
			xSemaphoreGive(sensorMUTEX);

			ResetMessageStruct(&currentRxMessage);
		}
	}
}

/*
 * This task reads the queue of characters from the Host PC when available
 * It then sends the processed data to the Sensor Controller Task
 */
void HostPC_RX_Task(){

	enum HostPCCommands HostPCCommand = PC_Command_NONE;

	pcMUTEX = xSemaphoreCreateMutex();

	Queue_HostPC_Data = xQueueCreate(80, sizeof(enum HostPCCommands));

	request_hostPC_read();

	while(1){
		HostPCCommand = parse_hostPC_message();

		if (HostPCCommand != PC_Command_NONE){
			xSemaphoreTake(pcMUTEX, portMAX_DELAY);
			xQueueSendToBack(Queue_HostPC_Data, &HostPCCommand, 0);
			xSemaphoreGive(pcMUTEX);

		}

	}
}