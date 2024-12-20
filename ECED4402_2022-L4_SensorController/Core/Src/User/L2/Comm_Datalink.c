/*
 * Comm_Datalink.c
 *
 *  Created on: Oct. 21, 2022
 *      Author: Andre Hendricks / Dr. JF Bousquet
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "User/L1/USART_Driver.h"
#include "User/L2/Comm_Datalink.h"
#include "User/util.h"

enum ParseMessageState_t {Waiting_S, SensorID_S, MessageID_S, ParamsID_S, Star_S, CS_S};

static void sendStringSensor(char* tx_string);

/******************************************************************************
******************************************************************************/
// Initializes the external sensor datalink
void initialize_sensor_datalink(void) {
    configure_usart_extern();
}

// Initializes the host PC datalink
void initialize_hostPC_datalink(void) {
    configure_usart_hostPC();
}

/******************************************************************************
Calculates the checksum and sends the input string via UART
******************************************************************************/
static void sendStringSensor(char* tx_string) {
    uint8_t checksum;
    uint16_t str_length;

    // Check the length of the command
    str_length = strlen((char *)tx_string) - 1;

    // Compute the checksum
    checksum = tx_string[0];
    for (int idx = 1; idx < str_length - 2; idx++) {
        checksum ^= tx_string[idx];
    }

    sprintf(&tx_string[str_length - 2], "%02x\n", checksum);
    printStr_extern(tx_string);
}

/******************************************************************************
Parses received sensor messages from the queue
******************************************************************************/
void parse_sensor_message(struct CommMessage* currentRxMessage) {
    static enum ParseMessageState_t currentState = Waiting_S;
    uint8_t CurrentChar;
    static uint16_t sensorIdIdx = 0, MessageIdIdx = 0, ParamIdx = 0, checksumIdx = 0;
    static char sensorId[6], CSStr[3];
    static uint8_t checksum_val;
    static const struct CommMessage EmptyMessage = {0};

    while (xQueueReceive(Queue_extern_UART, &CurrentChar, portMAX_DELAY) == pdPASS &&
           currentRxMessage->IsMessageReady == false) { // As long as there are characters in the queue
        if (CurrentChar == '$') { // Reset State Machine
            checksum_val = CurrentChar;
            sensorIdIdx = 0;
            MessageIdIdx = 0;
            ParamIdx = 0;
            checksumIdx = 0;
            currentState = SensorID_S;
            *currentRxMessage = EmptyMessage;
            continue;
        }

        // Calculate the checksum while parsing
        switch (currentState) {
            case Waiting_S: // Do nothing
                break;

            case SensorID_S: // Get Sensor ID
                checksum_val ^= CurrentChar;
                if (CurrentChar == ',') {
                    currentState = MessageID_S;
                    break;
                } else if (sensorIdIdx < 5) {
                    sensorId[sensorIdIdx++] = CurrentChar;
                }
                if (sensorIdIdx == 5) {
                    sensorId[sensorIdIdx] = '\0'; // Add NULL Terminator

                    if (strcmp(sensorId, "INFRA") == 0) { // Infrared Sensor ID
                        currentRxMessage->SensorID = Infrared;
                    } else if (strcmp(sensorId, "ULTRA") == 0) { // Ultrasonic Sensor ID
                        currentRxMessage->SensorID = Ultrasonic;
                    } else { // Unknown Sensor ID
                        currentRxMessage->SensorID = None;
                        currentState = Waiting_S;
                    }
                }
                break;

            case MessageID_S: // Get Message Type
                checksum_val ^= CurrentChar;
                if (CurrentChar == ',') {
                    currentState = ParamsID_S;
                } else if (MessageIdIdx < 2) {
                    currentRxMessage->messageId = currentRxMessage->messageId * 10;
                    currentRxMessage->messageId += CurrentChar - '0';
                    MessageIdIdx++;
                }
                break;

            case ParamsID_S: // Get Message Parameter
                checksum_val ^= CurrentChar;
                if (CurrentChar == ',') {
                    currentState = Star_S;
                } else if (ParamIdx < 8) {
                    currentRxMessage->params = currentRxMessage->params * 10;
                    currentRxMessage->params += CurrentChar - '0';
                    ParamIdx++;
                }
                break;

            case Star_S:
                checksum_val ^= CurrentChar;
                if (CurrentChar == ',') {
                    currentState = CS_S;
                }
                break;

            case CS_S:
                if (checksumIdx < 2) {
                    CSStr[checksumIdx++] = CurrentChar;
                }
                if (checksumIdx == 2) {
                    currentState = Waiting_S;
                    CSStr[checksumIdx] = '\0';
                    currentRxMessage->checksum = strtol(CSStr, NULL, 16);
                    if (currentRxMessage->checksum == checksum_val) {
                        currentRxMessage->IsMessageReady = true;
                        currentRxMessage->IsCheckSumValid = true;
                    } else {
                        currentRxMessage->IsCheckSumValid = false;
                    }
                }
                break;
        }
    }
}

/******************************************************************************
Parses messages from the host PC
******************************************************************************/
enum HostPCCommands parse_hostPC_message() {
    uint8_t CurrentChar;
    static char HostPCMessage[10];
    static uint16_t HostPCMessage_IDX = 0;

    while (xQueueReceive(Queue_hostPC_UART, &CurrentChar, portMAX_DELAY) == pdPASS) {
        if (CurrentChar == '\n' || CurrentChar == '\r' || HostPCMessage_IDX >= 6) {
            HostPCMessage[HostPCMessage_IDX++] = '\0';
            HostPCMessage_IDX = 0;
            if (strcmp(HostPCMessage, "START") == 0)
                return PC_Command_START;
            else if (strcmp(HostPCMessage, "RESET") == 0)
                return PC_Command_RESET;
        } else {
            HostPCMessage[HostPCMessage_IDX++] = CurrentChar;
        }
    }
    return PC_Command_NONE;
}

/******************************************************************************
Sends sensor data message for Infrared or Ultrasonic
******************************************************************************/
void send_sensorData_message(enum SensorId_t sensorType, uint16_t data) {
    char tx_sensor_buffer[50];

    switch (sensorType) {
        case Infrared:
            sprintf(tx_sensor_buffer, "$INFRA,03,%08u,*,00\n", data);
            break;
        case Ultrasonic:
            sprintf(tx_sensor_buffer, "$ULTRA,03,%08u,*,00\n", data);
            break;
        default:
            break;
    }
    sendStringSensor(tx_sensor_buffer);
}

/******************************************************************************
Sends enable message for Infrared or Ultrasonic
******************************************************************************/
void send_sensorEnable_message(enum SensorId_t sensorType, uint16_t TimePeriod_ms) {
    char tx_sensor_buffer[50];

    switch (sensorType) {
        case Infrared:
            sprintf(tx_sensor_buffer, "$INFRA,00,%08u,*,00\n", TimePeriod_ms);
            break;
        case Ultrasonic:
            sprintf(tx_sensor_buffer, "$ULTRA,00,%08u,*,00\n", TimePeriod_ms);
            break;
        default:
            break;
    }
    sendStringSensor(tx_sensor_buffer);
}

/******************************************************************************
Sends a reset message
******************************************************************************/
void send_sensorReset_message(void) {
    char tx_sensor_buffer[50];
    sprintf(tx_sensor_buffer, "$CNTRL,00,,*,00\n");
    sendStringSensor(tx_sensor_buffer);
}

void send_ack_message(enum AckTypes AckType){
    char tx_sensor_buffer[50];

    switch(AckType){
        case UltrasonicSensorEnable:
            sprintf(tx_sensor_buffer, "$ULTRA,01,,*,00\n");
            break;
        case InfraredSensorEnable:
            sprintf(tx_sensor_buffer, "$INFRA,01,,*,00\n");
            break;
        case RemoteSensingPlatformReset:
            sprintf(tx_sensor_buffer, "$CNTRL,01,,*,00\n");
            break;
        default:
            // Handle unknown ack type
            return;
    }

    sendStringSensor(tx_sensor_buffer);
}
