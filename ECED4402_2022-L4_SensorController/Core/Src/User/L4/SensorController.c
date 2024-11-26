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

//Required FreeRTOS header files
#include "FreeRTOS.h"
#include "Timers.h"
#include "semphr.h"

QueueHandle_t Queue_Sensor_Data;
QueueHandle_t Queue_HostPC_Data;


static void ResetMessageStruct(struct CommMessage* currentRxMessage){

	static const struct CommMessage EmptyMessage = {0};
	*currentRxMessage = EmptyMessage;
}

/******************************************************************************
This task is created from the main.
******************************************************************************/
void SensorControllerTask(void *params) {

    ControllerState_t current_state = INIT_STATE;  // Initialize the state to INIT_STATE
    SensorStatus_t sensor_status = {false, false, NULL};  // Initialize sensor status
    enum HostPCCommands pc_command;  // Variable to store commands from Host PC
    struct CommMessage sensor_msg;  // Variable to store messages from sensors
    
    // Create acknowledgment timer with 1 second timeout, no auto-reload, and no callback function
    sensor_status.ack_timer = xTimerCreate(
        "AckTimer",
        pdMS_TO_TICKS(1000),  // 1 second timeout
        pdFALSE,  // Don't auto-reload
        NULL,
        NULL  // Timer callback function can be added if needed
    );

    while(1) {
        switch(current_state) {

			case INIT_STATE:
				// Wait for START command from Host PC
				if (xQueueReceive(Queue_HostPC_Data, &pc_command, pdMS_TO_TICKS(100)) == pdPASS) {
					if (pc_command == PC_Command_START) {
						print_str("START command received\r\n");
						// Reset sensor status flags
						sensor_status.acoustic_enabled = false;
						sensor_status.depth_enabled = false;
						current_state = START_SENSORS_STATE;  // Transition to START_SENSORS_STATE
					}
				}
				break;
                
			case START_SENSORS_STATE:
				if (!sensor_status.acoustic_enabled || !sensor_status.depth_enabled) {
					// Send enable commands to sensors if they are not enabled
					if (!sensor_status.acoustic_enabled) {
						send_sensorEnable_message(Acoustic, 5000);  // 5 second period
					}
					if (!sensor_status.depth_enabled) {
						send_sensorEnable_message(Depth, 2000);    // 2 second period
					}
					
					// Start acknowledgment timer
					xTimerStart(sensor_status.ack_timer, portMAX_DELAY);
					
					// Check sensor responses
					if (xQueueReceive(Queue_Sensor_Data, &sensor_msg, pdMS_TO_TICKS(100)) == pdPASS) {
						if (sensor_msg.IsMessageReady && sensor_msg.IsCheckSumValid) {
							if (sensor_msg.messageId == 1) {  // Acknowledgment message
								if (sensor_msg.SensorID == Acoustic) {
									sensor_status.acoustic_enabled = true;
								} else if (sensor_msg.SensorID == Depth) {
									sensor_status.depth_enabled = true;
								}
							}
						}
					}

					// Both sensors enabled -> move to PARSE_SENSOR_DATA_STATE
					if (sensor_status.acoustic_enabled && sensor_status.depth_enabled) {
						current_state = PARSE_SENSOR_DATA_STATE;
						print_str("All sensors enabled\r\n");
					}
				}
				break;
                
			case PARSE_SENSOR_DATA_STATE:
				// Check for RESET command from Host PC
				if (xQueueReceive(Queue_HostPC_Data, &pc_command, 0) == pdPASS) {
					if (pc_command == PC_Command_RESET) {
						print_str("RESET command received\r\n");
						current_state = DISABLE_SENSORS_STATE;  // Transition to DISABLE_SENSORS_STATE
						break;
					}
				}

				// Process sensor data
				if (xQueueReceive(Queue_Sensor_Data, &sensor_msg, pdMS_TO_TICKS(100)) == pdPASS) {
					if (sensor_msg.IsMessageReady && sensor_msg.IsCheckSumValid) {
						if (sensor_msg.messageId == 3) {  // Data message
							// Forward sensor data to Host PC
							switch(sensor_msg.SensorID) {
								case Acoustic:
									print_str("Acoustic data: ");
									// Convert data to string and print
									char data_str[50];
									sprintf(data_str, "%d\r\n", sensor_msg.params);
									print_str(data_str);
									break;
									
								case Depth:
									print_str("Depth data: ");
									sprintf(data_str, "%d\r\n", sensor_msg.params);
									print_str(data_str);
									break;
							}
						}
					}
				}
				break;

			case DISABLE_SENSORS_STATE:
				// Send reset command to sensor platform
				send_sensorReset_message();
				
				// Reset sensor status
				sensor_status.acoustic_enabled = false;
				sensor_status.depth_enabled = false;
				
				// Stop acknowledgment timer if running
				xTimerStop(sensor_status.ack_timer, portMAX_DELAY);
				
				print_str("Sensors disabled\r\n");
				
				// Return to initial state
				current_state = INIT_STATE;
				break;
        }
        
        vTaskDelay(pdMS_TO_TICKS(10)); // Prevent task starvation
    }
}




/*
 * This task reads the queue of characters from the Sensor Platform when available
 * It then sends the processed data to the Sensor Controller Task
 */
void SensorPlatform_RX_Task(){
	struct CommMessage currentRxMessage = {0};
	Queue_Sensor_Data = xQueueCreate(80, sizeof(struct CommMessage));

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

	Queue_HostPC_Data = xQueueCreate(80, sizeof(enum HostPCCommands));

	request_hostPC_read();

	while(1){
		HostPCCommand = parse_hostPC_message();

		if (HostPCCommand != PC_Command_NONE){
			xQueueSendToBack(Queue_HostPC_Data, &HostPCCommand, 0);
		}

	}
}
