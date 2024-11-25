# Marine Debris Detection System

Real-time marine debris detection system using STM32F411RE and FreeRTOS. Implements sensor data acquisition, UART communication, and multi-board control architecture for underwater environment monitoring.

## Features
- Dual STM32F411RE board configuration
- FreeRTOS real-time task management
- UART communication with NMEA-style protocol
- Acoustic and depth sensor integration
- Host PC interface for control and monitoring

## Hardware Requirements
- 2x STM32F411RE NUCLEO boards
- UART connections between boards
- Ultrasonic sensor
- IR sensor
- Power supply

## Software Requirements
- STM32CubeIDE 1.16.1
- FreeRTOS
- STM32 HAL drivers

## Project Structure
```
├── Core/Src/
│   ├── User/
│   │   ├── L1/        # Hardware drivers
│   │   ├── L2/        # Communication datalink
│   │   ├── L3/        # Sensor implementations
│   │   └── L4/        # Platform and controller logic
│   └── main.c         # Main application
```


## Setup Instructions
1. Clone repository
2. Import project in STM32CubeIDE
3. Configure UART pins
4. Build and flash to both boards
5. Connect a serial terminal to monitor using PuTTY

## Usage
1. Power on both boards
2. Send "START" from the host PC to initiate
3. Monitor sensor data through serial output
4. Send "RESET" to stop system

## License
[License Type]

## Contributors
- Iftekhar Hossain Rafi
- Mazen Abdalla
