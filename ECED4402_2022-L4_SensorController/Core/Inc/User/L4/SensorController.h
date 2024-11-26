/*
	SensorController.h
 
   Created on: November 26, 2024
       Author: Iftekhar
 */

#ifndef INC_USER_L4_SENSORCONTROLLER_H_
#define INC_USER_L4_SENSORCONTROLLER_H_

// Structure to hold the states of various sensors
struct SensorStates{
	bool IsAcousticAck;    // Indicates if the acoustic sensor has acknowledged
	bool IsDepthAck;       // Indicates if the depth sensor has acknowledged
	uint16_t DepthData;    // Holds the data from the depth sensor
	uint16_t AcousticData; // Holds the data from the acoustic sensor
};

// Enumeration for the different states of the sensor controller
typedef enum {
    INIT_STATE,              // Initial state
    START_SENSORS_STATE,     // State to start the sensors
    PARSE_SENSOR_DATA_STATE, // State to parse the sensor data
    DISABLE_SENSORS_STATE    // State to disable the sensors
} ControllerState_t;

// Function prototypes for tasks related to sensor control
void HostPC_RX_Task();          // Task to handle data reception from the host PC
void SensorPlatform_RX_Task();  // Task to handle data reception from the sensor platform
void SensorControllerTask(void *params); // Main task for controlling the sensors

#endif /* INC_USER_L4_SENSORCONTROLLER_H_ */
