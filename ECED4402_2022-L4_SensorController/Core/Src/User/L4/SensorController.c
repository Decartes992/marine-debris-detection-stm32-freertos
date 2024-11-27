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

#include "User/L3/AcousticSensor.h"
#include "User/L3/CorrosionSensor.h"
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
	int Acoustic_enabled = 0, Ultrasonic_enabled = 0, FlowRate_enabled = 0;
	int Corrosion_enabled = 0, Disabled = 0;
	int Infrared_enabled = 0;
	int sensorDataCounter = 0;
	enum states state;
	enum HostPCCommands HostPCCommand;
	char str[60];
	char strAcoustic[100];
	char strUltrasonic[100];
	char strFlowRate[100];
	char strCorrosion[100];
	char strInfrared[100];
	char *AcousticStatus;
	char *UltrasonicStatus;
	char *FlowRateStatus;
	char *CorrosionStatus;
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
				sprintf(str, "Running Sensors\r\n");
				print_str(str);

				send_sensorEnable_message(Acoustic, 1000);
				send_sensorEnable_message(Ultrasonic, 2000);
				send_sensorEnable_message(FlowRate, 3000);
				send_sensorEnable_message(Infrared, 4000);

				xTimerStart(xTimer, 0);

				while(!Sensors_Expired) {
					if(xQueueReceive(Queue_Sensor_Data, &currentRxMessage, 0) == pdPASS) {
						if(currentRxMessage.messageId == 1) {
							switch(currentRxMessage.SensorID) {
								case Acoustic:
									Acoustic_enabled = 1;
									break;
								case Ultrasonic:
									Ultrasonic_enabled = 1;
									break;
								case FlowRate:
									FlowRate_enabled = 1;
									break;
								case Corrosion:
									Corrosion_enabled = 1;
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
				Corrosion_enabled = 1;

				if(Acoustic_enabled && Ultrasonic_enabled && FlowRate_enabled && Corrosion_enabled && Infrared_enabled) {
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
								case Acoustic:
									AcousticStatus = analyzeAcousticValue(currentRxMessage.params);
									sprintf(strAcoustic, "Acoustic Sensor Data: %d dB - Status: %s\r\n", 
											currentRxMessage.params, AcousticStatus);
									break;
								case Ultrasonic:
									UltrasonicStatus = analyzeUltrasonicValue(currentRxMessage.params);
									sprintf(strUltrasonic, "Ultrasonic Sensor Data: %.2f cm - Status: %s\r\n", 
											currentRxMessage.params, UltrasonicStatus);
									break;
								case FlowRate:
									FlowRateStatus = analyzeFlowRateValue(currentRxMessage.params);
									sprintf(strFlowRate, "Flow Rate Sensor Data: %d L/min - Status: %s\r\n", 
											currentRxMessage.params, FlowRateStatus);
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

				switch (sensorDataCounter % 4) {
					case 0:
						print_str(strAcoustic);
						break;
					case 1:
						print_str(strUltrasonic);
						break;
					case 2:
						print_str(strFlowRate);
						break;
					case 3:
						print_str(strInfrared);
						break;
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

// Analyze Acoustic data for oil pipeline monitoring
char* analyzeAcousticValue(int acousticValue) {
    if (acousticValue < 50) {
        return "Normal Operation - No Leakage Detected";
    } else if (acousticValue >= 50 && acousticValue <= 80) {
        return "Caution - Possible Disturbance";
    } else {
        return "Alert - Potential Leakage or Structural Issue Detected";
    }
}

// Analyze Pressure data for oil pipeline monitoring
char* analyzePressureValue(int pressureValue) {
    if (pressureValue < 100) {
        return "Low Pressure - Possible Leakage Detected";
    } else if (pressureValue > 200) {
        return "High Pressure - Risk of Pipeline Rupture";
    } else {
        return "Normal Pressure - Pipeline Operating Safely";
    }
}

// Analyze Flow Rate data for oil pipeline monitoring
char* analyzeFlowRateValue(int flowRateValue) {
    if (flowRateValue < 100) {
        return "Low Flow - Possible Blockage or Leakage";
    } else if (flowRateValue > 200) {
        return "High Flow - Potential Overload or Equipment Malfunction";
    } else {
        return "Normal Flow - Pipeline Operating Within Expected Parameters";
    }
}

// Analyze Corrosion data for oil pipeline monitoring
char* analyzeCorrosionValue(int corrosionValue) {
    if (corrosionValue < 30) {
        return "Low Corrosion - Good Condition";
    } else if (corrosionValue >= 30 && corrosionValue <= 60) {
        return "Moderate Corrosion - Maintenance Recommended";
    } else {
        return "High Corrosion - Immediate Attention Required";
    }
}

// Analyze Infrared data for oil pipeline monitoring
char* analyzeInfraredValue(int infraredValue) {
    if (infraredValue < 50) {
        return "Normal Operation - No Obstruction Detected";
    } else if (infraredValue >= 50 && infraredValue <= 80) {
        return "Caution - Possible Obstruction";
    } else {
        return "Alert - Obstruction Detected";
    }
}
