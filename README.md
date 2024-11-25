<p><a target="_blank" href="https://app.eraser.io/workspace/PC4XAUotFmEPGhxQ497y" id="edit-in-eraser-github-link"><img alt="Edit in Eraser" src="https://firebasestorage.googleapis.com/v0/b/second-petal-295822.appspot.com/o/images%2Fgithub%2FOpen%20in%20Eraser.svg?alt=media&amp;token=968381c8-a7e7-472a-8ed6-4a6626da5501"></a></p>

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


# System Overview
![image.png](/.eraser/PC4XAUotFmEPGhxQ497y___mOJMEuxMFFO7MZ2mGvGVJzb7uOn2___zMsryLpSMHmAAfhpyDJr-.png "image.png")



 

## License
[License Type]

## Contributors
- Iftekhar Hossain Rafi
- Mazen Abdalla



<!-- eraser-additional-content -->
## Diagrams
<!-- eraser-additional-files -->
<a href="/README-System Overview-1.eraserdiagram" data-element-id="dmEV7m3HXGZm0QbaWk4VE"><img src="/.eraser/PC4XAUotFmEPGhxQ497y___mOJMEuxMFFO7MZ2mGvGVJzb7uOn2___---diagram----5bc39187af56c70b2411e513e61a0b9d-System-Overview.png" alt="" data-element-id="dmEV7m3HXGZm0QbaWk4VE" /></a>
<!-- end-eraser-additional-files -->
<!-- end-eraser-additional-content -->
<!--- Eraser file: https://app.eraser.io/workspace/PC4XAUotFmEPGhxQ497y --->