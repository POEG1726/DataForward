/*
    TCPServer.h
    Created on Feb 21, 2025
    Author: @POEG1726
*/

#ifndef _TCPSERVER_H_
#define _TCPSERVER_H_

typedef enum
{
    CW,
    CCW
} MotorDir;

typedef struct
{
    float accel_x;
    float accel_y;
    float accel_z;
    float gyro_x;
    float gyro_y;
    float gyro_z;
    float Temperature;
} ICM42688_Data_t;

typedef struct
{
    int WifiSignalStrength;
    float Voltage;
    float Temperature;
    ICM42688_Data_t IMUData;
    struct
    {
        float Speed;
        MotorDir Direction; // CW 或 CCW
        float Amps;
    } Motor[CONFIG_MOTOR_COUNT];
} SensorData_t;

void Init_WiFi(void);

#endif // _TCPSERVER_H_