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

// Resets the communication message structure
static void ResetMessageStruct(struct CommMessage* currentRxMessage) {
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
    // Initialize variables
    const TickType_t TimerDefaultPeriod = 10000;
    TimerHandle_t TimerID_UltrasonicSensor, TimerID_InfraredSensor;
    struct CommMessage currentRxMessage = {0};
    char str[100];  // For debug messages

    // Create timers for Ultrasonic and Infrared sensors
    TimerID_UltrasonicSensor = xTimerCreate(
        "Ultrasonic Sensor Timer", 
        TimerDefaultPeriod, 
        pdTRUE, 
        (void *)1, 
        RunUltrasonicSensor
    );

    TimerID_InfraredSensor = xTimerCreate(
        "Infrared Sensor Timer", 
        TimerDefaultPeriod, 
        pdTRUE, 
        (void *)2,
        RunIRSensor
    );

    // Start UART reading
    request_sensor_read();  // requests a usart read (through the callback)

    sprintf(str, "Sensor Platform initialized and waiting for commands...\r\n");
    print_str(str);

    do {
        parse_sensor_message(&currentRxMessage);

        if(currentRxMessage.IsMessageReady == true && currentRxMessage.IsCheckSumValid == true) {
            sprintf(str, "Received message: SensorID=%d, MessageID=%d, Params=%d\r\n",
                    currentRxMessage.SensorID, currentRxMessage.messageId, currentRxMessage.params);
            print_str(str);

            switch(currentRxMessage.SensorID) {
                case Controller:
                    switch(currentRxMessage.messageId) {
                        case 0:  // Reset command
                            sprintf(str, "Stopping all sensors\r\n");
                            print_str(str);
                            xTimerStop(TimerID_UltrasonicSensor, portMAX_DELAY);
                            xTimerStop(TimerID_InfraredSensor, portMAX_DELAY);
                            send_ack_message(RemoteSensingPlatformReset);
                            break;
                        case 1:  // Acknowledgment
                        case 3:  // Data message
                            // Do nothing for these message types
                            break;
                    }
                    break;

                case Ultrasonic:
                    switch(currentRxMessage.messageId) {
                        case 0:  // Enable command
                            sprintf(str, "Enabling Ultrasonic sensor with period %d\r\n",
                                    currentRxMessage.params);
                            print_str(str);
                            xTimerChangePeriod(TimerID_UltrasonicSensor,
                                             currentRxMessage.params,
                                             portMAX_DELAY);
                            xTimerStart(TimerID_UltrasonicSensor, portMAX_DELAY);
                            send_ack_message(UltrasonicSensorEnable);
                            break;
                        case 1:  // Acknowledgment
                        case 3:  // Data message
                            // Do nothing for these message types
                            break;
                    }
                    break;

                case Infrared:
                    switch(currentRxMessage.messageId) {
                        case 0:  // Enable command
                            sprintf(str, "Enabling Infrared sensor with period %d\r\n",
                                    currentRxMessage.params);
                            print_str(str);
                            xTimerChangePeriod(TimerID_InfraredSensor,
                                             currentRxMessage.params,
                                             portMAX_DELAY);
                            xTimerStart(TimerID_InfraredSensor, portMAX_DELAY);
                            send_ack_message(InfraredSensorEnable);
                            break;
                        case 1:  // Acknowledgment
                        case 3:  // Data message
                            // Do nothing for these message types
                            break;
                    }
                    break;

                default:
                    sprintf(str, "Unknown sensor ID: %d\r\n", currentRxMessage.SensorID);
                    print_str(str);
                    break;
            }

            ResetMessageStruct(&currentRxMessage);
        }
    } while(1);
}
