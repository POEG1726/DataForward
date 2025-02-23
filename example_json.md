# JSON Content Explanation

## Type: Data, Console

### Data
- 传输小车相关数据
- **WifiSignalStrength**: WiFi信号强度, 单位db
- **Voltage**: 总线电压, 单位V
- **Temperature**: 环境温度, 单位°C
- **Accel**: 加速度, 包含XYZ三轴, 单位m/s²
- **Gyro**: 三轴角速度
- **Motor_?**: 电机?数据
  - **Speed**: 转速, 单位rpm
  - **Direction**: 旋转方向, CW 或 CCW
  - **Amps**: 电机电流, 单位A
- **Example**
```
{
    "type": "data",
    "data": {
        "WifiSignalStrength": -50,
        "Voltage": 12.3,
        "Temperature": 30.0,
        "Accel": {
            "x": 0.1,
            "y": 0.2,
            "z": 0.3
        },
        "Gyro": {
            "x": 0.4,
            "y": 0.5,
            "z": 0.6
        },
        "Motor_1": {
            "Speed": 0.7,
            "Direction": "CW",
            "Amps": 0.9
        },
        "Motor_2": {
            "Speed": 0.7,
            "Direction": "CW",
            "Amps": 0.9
        },
        "Motor_3": {
            "Speed": 0.7,
            "Direction": "CW",
            "Amps": 0.9
        },
        "Motor_4": {
            "Speed": 0.7,
            "Direction": "CW",
            "Amps": 0.9
        }
    }
}
```

### Console
- **Dir**: 消息方向, SendToController& ReiceivedFromController
- **Msg**: 消息内容
- **Example**
```
{
    "type": "Console",
    "Dir": "SendToController",
    "Msg": "move 0 D WA 100 0"//WA左前方45°,运行100距离. 距离模式时time参数无效

    //运行成功返回1,失败返回0
}
{
    "type": "Console",
    "Dir": "ReceivedFromController",
    "msg": "0"
}
```
```
当前指令列表:
move [Stop] [S/D] [WASD] [Value] [Time] #Stop:是否急停 S/D:速度/距离, WASD:朝向,前后左右,可组合, Value:值, Time:如果为速度模式,运行时间,0无限
spin [L/R] [Angle] #L/R:左右, Angel:角度
motor [MotorID] [Dir] [Angle] 
```
