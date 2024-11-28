/*
 * SensorController.c
 *
 *  Created on: Oct 24, 2022
 *      Author: kadh1
 */


#include <stdio.h>
#include <User/L3/InfraredSensor.h>
#include <User/L3/UltrasonicSensor.h>

#include "main.h"

#include "User/L2/Comm_Datalink.h"

#include "User/L4/SensorPlatform.h"
#include "User/L4/SensorController.h"

#include "User/util.h"

//Required FreeRTOS header files
#include "FreeRTOS.h"
#include "Timers.h"
#include "semphr.h"

QueueHandle_t Queue_Sensor_Data;
QueueHandle_t Queue_HostPC_Data;

int Sensors_Expired = 0;

TimerHandle_t xTimer;

enum states {Start_Sensors, Parse_Sensor_Data, Disable_Sensors, Wait_};
char states_str[3][6] = {"EMPTY", "START", "RESET"};

static void ResetMessageStruct(struct CommMessage* currentRxMessage){

	static const struct CommMessage EmptyMessage = {0};
	*currentRxMessage = EmptyMessage;
}

void CheckEnableSensor( TimerHandle_t xTimer )
{
	Sensors_Expired = 1;

}

/**************************
This task is created from the main.
**************************/
void SensorControllerTask(void *params)
{
	// All variable declarations at the start
	struct CommMessage currentRxMessage = {0};
	int Ultrasonic_enabled = 0, Infrared_enabled = 0;
	int sensorDataCounter = 0;
	enum states state;
	enum HostPCCommands HostPCCommand;
	char str[60];
	char strUltrasonic[100];
	char strInfrared[100];
	char *UltrasonicStatus;
	char *InfraredStatus;

	// Initialize variables
	state = Wait_;
	HostPCCommand = PC_Command_NONE;

	xTimer = xTimerCreate("Timer1", 5000, pdTRUE, (void *)0, CheckEnableSensor);

	do {
		switch(state) {
			case Wait_:
				sprintf(str, "Polling\r\n");
				print_str(str);

				if(xQueueReceive(Queue_HostPC_Data, &HostPCCommand, 0) == pdPASS) {
					sprintf(str, "Prompt from host: %s\r\n", states_str[HostPCCommand]);
					print_str(str);
					if(HostPCCommand == PC_Command_START) {
						state = Start_Sensors;
						Sensors_Expired = 0;
					}
				} else {
					state = Wait_;
					Sensors_Expired = 0;
				}
				break;

			case Start_Sensors:

			    sprintf(str, "Sending enable messages...\r\n");
			    print_str(str);

			    send_sensorEnable_message(Ultrasonic, 5000);
			    sprintf(str, "Sent Ultrasonic enable\r\n");
			    print_str(str);

			    send_sensorEnable_message(Infrared, 4000);
			    sprintf(str, "Sent Infrared enable\r\n");
			    print_str(str);
				xTimerStart(xTimer, 0);

				while(!Sensors_Expired) {
					if(xQueueReceive(Queue_Sensor_Data, &currentRxMessage, 0) == pdPASS) {
						if(currentRxMessage.messageId == 1) {
							switch(currentRxMessage.SensorID) {
								case Ultrasonic:
									Ultrasonic_enabled = 1;
									break;
								case Infrared:
									Infrared_enabled = 1;
									break;
								default:
									break;
							}
						}
					}
					ResetMessageStruct(&currentRxMessage);
				}

				xTimerStop(xTimer, 0);

				if(Ultrasonic_enabled && Infrared_enabled) {
					state = Parse_Sensor_Data;
					Sensors_Expired = 0;
				} else {
					state = Start_Sensors;
					Sensors_Expired = 0;
				}
				break;

			case Parse_Sensor_Data:
				sprintf(str, "Processing Sensor Data\r\n");
				print_str(str);

				xTimerStart(xTimer, 0);

				while(!Sensors_Expired) {
					if(xQueueReceive(Queue_Sensor_Data, &currentRxMessage, 0) == pdPASS) {
						if(currentRxMessage.messageId == 3) {
							switch(currentRxMessage.SensorID) {
								case Ultrasonic:
									UltrasonicStatus = analyzeUltrasonicValue(currentRxMessage.params);
									sprintf(strUltrasonic, "Ultrasonic Sensor Data: %d cm - Status: %s\r\n", 
											currentRxMessage.params, UltrasonicStatus);
									break;
								case Infrared:
									InfraredStatus = analyzeInfraredValue(currentRxMessage.params);
									sprintf(strInfrared, "Infrared Sensor Data: %d - Status: %s\r\n", 
											currentRxMessage.params, InfraredStatus);
									break;
								default:
									break;
							}
						}
					}
				}

				if(sensorDataCounter % 2 == 0) {
					print_str(strUltrasonic);
				} else {
					print_str(strInfrared);
				}

				sensorDataCounter++;
				xTimerStop(xTimer, 0);
				print_str(str);

				if(xQueueReceive(Queue_HostPC_Data, &HostPCCommand, 0) == pdPASS) {
					sprintf(str, "Prompt from host: %s\r\n", states_str[HostPCCommand]);
					print_str(str);
					if(HostPCCommand == PC_Command_RESET) {
						state = Disable_Sensors;
						Sensors_Expired = 0;
					}
				} else {
					state = Parse_Sensor_Data;
					Sensors_Expired = 0;
				}
				break;

			case Disable_Sensors:
				sprintf(str, "Stopping sensors\r\n");
				print_str(str);
				send_sensorReset_message();
				state = Wait_;
				break;
		}
	} while(1);
}


/*
 * This task reads the queue of characters from the Sensor Platform when available
 * It then sends the processed data to the Sensor Controller Task
 */
void SensorPlatform_RX_Task(){
	struct CommMessage currentRxMessage = {0};
	Queue_Sensor_Data = xQueueCreate(200, sizeof(struct CommMessage));

	request_sensor_read();  // requests a usart read (through the callback)

	while(1){
		parse_sensor_message(&currentRxMessage);

		if(currentRxMessage.IsMessageReady == true && currentRxMessage.IsCheckSumValid == true){

			xQueueSendToBack(Queue_Sensor_Data, &currentRxMessage, 0);
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

	Queue_HostPC_Data = xQueueCreate(200, sizeof(enum HostPCCommands));

	request_hostPC_read();

	while(1){
		HostPCCommand = parse_hostPC_message();

		if (HostPCCommand != PC_Command_NONE){
			xQueueSendToBack(Queue_HostPC_Data, &HostPCCommand, 0);
		}

	}
}

// Analyze Ultrasonic data for oil pipeline monitoring
char* analyzeUltrasonicValue(int ultrasonicValue) {
    if (ultrasonicValue > 80) {
        return "Normal Operation - No Obstruction Detected";
    } else if (ultrasonicValue >= 10 && ultrasonicValue <= 80) {
        return "Caution - Possible Obstruction";
    } else {
        return "Alert - Obstruction Detected";
    }
}

// Analyze Infrared data for oil pipeline monitoring
char* analyzeInfraredValue(int infraredValue) {
    switch (infraredValue) {
        case PLASTIC_DEBRIS:
            return "Plastic material detected";
        case ORGANIC_DEBRIS:
            return "Organic material detected";
        case METAL_DEBRIS:
            return "Metal debris detected";
        case UNKNOWN_DEBRIS:
            return "Unknown material detected";
        default:
            return "No significant heat signature detected";
    }
}
