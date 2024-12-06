# Robotics Project - Snake Game Control with IMU and LCD

## Description

In this project, we will use an IMU sensor communicating via I2C to control the Snake game on an LCD screen using the SPI interface. The IMU sensor will detect the user's movements, and these data will be used to control the snake on the LCD screen.

## *Hardware Design*

### Components

| Component                          | Documentation Link                                                                                                           |
|------------------------------------|-----------------------------------------------------------------------------------------------------------------------------|
| uC Nano V3.0 Atmega328             | [Datasheet](https://docs.rs-online.com/4ccc/A700000011593182.pdf)                                                            |
| MPU6050 Accelerometer Gyroscope Module | [Documentation](https://docs.sunfounder.com/projects/ultimate-sensor-kit/en/latest/components_basic/05-component_mpu6050.html) |
| 3.3V Voltage Regulator AMS1117     | [Datasheet](http://www.advanced-monolithic.com/pdf/ds1117.pdf)                                                               |
| 2.4 inch LCD Touchscreen           | [User Manual](http://www.lcdwiki.com/res/MAR2406/2.4inch_Arduino_8BIT_Module_MAR2406_User_Manual_EN.pdf)                     |
| Analog Joystick                    | N/A                                                                                                                         |
| Buzzer                             | N/A                                                                                                                         |
| Resistors, Wires, etc.             | N/A                                                                                                                         |

### Circuit Diagram


## *Software Design*

### Development Environment

- [PlatformIO](https://platformio.org/)
- [VSCode](https://code.visualstudio.com/)

### *Libraries Used*


### *Code Description*

The code reads data from the IMU sensor via I2C and updates the snake's position on the LCD screen using the SPI interface. The user's movements are translated into commands for the Snake game.
