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

typedef enum
{
    CMD_MOVE,
    CMD_SPIN,
    CMD_MOTOR,
    CMD_UNKNOWN
} CommandType;

typedef struct
{
    int stop;         // move 命令中的 Stop 参数（是否急停，0表示不急停，1表示急停）
    char sd;          // move 命令中的 S/D 参数（例如用字符 'S' 表示速度模式，'D' 表示距离模式）
    char wasd[16];    // move 命令中的方向参数，可能是组合的 "W", "A", "S", "D"
    int value;        // move 命令中的 Value 参数
    int time;         // move 命令中的 Time 参数（如果为速度模式，运行时间；0表示无限）
} MoveParams;

typedef struct
{
    char lr;          // spin 命令中的 L/R 参数， 'L' 或 'R'
    int angle;        // spin 命令中的角度参数
} SpinParams;

typedef struct
{
    int motorID;      // motor 命令中的 MotorID
    char dir;         // motor 命令中的 Dir 参数，'C'（CW）或 'C'（CCW），可根据需要修改
    int angle;        // motor 命令中的 Angle 参数
} MotorParams;

// 使用联合体存储不同类型命令的参数
typedef union
{
    MoveParams move;
    SpinParams spin;
    MotorParams motor;
} CommandParams;

// 定义最终的命令结构体
typedef struct
{
    CommandType type;
    CommandParams params;
} Command;


void Init_WiFi(void);
void Init_TCPServer(void);
Command parse_command(const char* msg);

#endif // _TCPSERVER_H_