# ESP32 TCP Server & Command Processing Project

This project implements a TCP server on the ESP32 using ESP-IDF and FreeRTOS. It supports simultaneous communication with multiple clients (up to 3) via IPv4, broadcasts sensor data (in JSON format) to all connected clients, and processes commands received from clients. The commands are received as JSON (with a structure like `{ "type": "Console", "Msg": "move 0 D WA 100 0" }`), parsed into a binary structure, and then sent out via UART as raw binary data.

本项目在 ESP32 上使用 ESP-IDF 和 FreeRTOS 实现了一个 TCP 服务器。它支持 IPv4 下最多与 3 个客户端同时通信，通过 TCP 广播传感器数据（JSON 格式）给所有连接的客户端，并处理来自客户端的命令。客户端的命令以 JSON 结构（例如 `{ "type": "Console", "Msg": "move 0 D WA 100 0" }`）发送，解析后转换为二进制结构，通过 UART 以原始二进制数据方式发送出去。

## Features / 特性

- **Wi-Fi Initialization and Best AP Selection**  
  The ESP32 scans for available APs and connects to one based on the best signal (supporting two target SSIDs).  
  ESP32 扫描可用 AP，并根据最佳信号连接到目标 AP（支持两个目标 SSID）。

- **Multi-Client TCP Server**  
  A TCP server listens on a port defined by `CONFIG_SERVER_PORT` and supports up to 3 simultaneous IPv4 client connections.  
  TCP 服务器在 `CONFIG_SERVER_PORT` 定义的端口监听，支持最多 3 个同时连接的 IPv4 客户端。

- **Sensor Data Processing and Broadcasting**  
  Sensor data (of type `SensorData_t`) is received through a UART queue, processed into a JSON object, and then broadcast to all connected clients.  
  传感器数据通过 UART 队列接收（类型为 `SensorData_t`），处理后转换为 JSON 对象，并广播给所有已连接的客户端。

- **Command Parsing and UART Transmission**  
  Client commands in JSON format are parsed into a `Command` structure and then sent as raw binary data via UART.  
  客户端以 JSON 格式发送的命令被解析为 `Command` 结构体，然后以二进制数据形式通过 UART 发送出去。

- **Resource Optimization**  
  Tasks for command parsing, client data processing, TCP server initialization, and sensor data processing are started only after Wi-Fi is connected to save resources.  
  为节省资源，命令解析、客户端数据处理、TCP 服务器初始化以及传感器数据处理任务仅在 Wi-Fi 连接成功后启动。

- **LED Indicator**  
  An LED indicator is used to show system status (e.g., Wi-Fi connection, error states). The LED pin and behavior can be configured via menuconfig.  
  使用 LED 指示灯显示系统状态（例如 Wi-Fi 连接状态、错误状态）。LED 的管脚和行为可通过 menuconfig 进行配置。

## Project Structure / 项目结构

- **TCPServer.c**  
  Contains the TCP server code, which creates the socket, accepts client connections, spawns individual tasks for each client, and broadcasts sensor data.  
  包含 TCP 服务器代码，创建 socket、接受客户端连接，并为每个客户端创建独立任务，同时广播传感器数据。

- **Command Parsing Functions**  
  Implements functions to parse client command strings (received in JSON format) into a binary structure.  
  实现将客户端命令字符串（以 JSON 格式发送）解析为二进制结构的函数。

- **Process_Data Task**  
  Processes sensor data received from the UART queue, converts it into JSON, and broadcasts it to all connected TCP clients.  
  处理从 UART 队列中接收到的传感器数据，转换为 JSON 后广播给所有 TCP 客户端。

- **UART Communication Module (user_uart.c/h)**  
  Contains UART initialization and sending functions.  
  包含 UART 初始化和发送函数。

## How It Works / 工作原理

1. **Wi-Fi Connection:**  
   The ESP32 initializes Wi-Fi in STA mode, scans for available APs, and connects to the best one according to preconfigured target SSIDs and signal strength.  
   ESP32 以 STA 模式初始化 Wi-Fi，扫描可用 AP，并根据预设目标 SSID 与信号强度连接到最佳 AP。

2. **TCP Server Operation:**  
   Once Wi-Fi is connected, the TCP server is started. It listens for client connections and creates separate tasks to handle each client.  
   Wi-Fi 连接成功后，启动 TCP 服务器。服务器监听客户端连接，并为每个连接创建独立任务进行处理。

3. **Sensor Data Processing and Broadcasting:**  
   Sensor data is collected via UART and placed into a queue. The Process_Data task waits for new sensor data, converts it into a JSON string, and then broadcasts it to all connected clients.  
   传感器数据通过 UART 收集后放入队列，Process_Data 任务等待数据到来，将其转换为 JSON 字符串，并广播给所有已连接的客户端。

4. **Command Reception and Processing:**  
   Client tasks receive data from their respective sockets. When a command (in JSON format) is received, it is parsed into a Command structure. The command is then sent via UART as binary data.  
   每个客户端任务从对应的 socket 接收数据。当收到命令（JSON 格式）时，解析为 Command 结构体，然后通过 UART 以二进制数据形式发送出去。

## How to Build and Flash / 编译与烧录

1. **Configure Your Project:**  
   Use `menuconfig` to adjust settings such as `CONFIG_SERVER_PORT`, Wi-Fi SSID/Password, LED pin configuration, and other relevant settings. See `Kconfig.projbuild` for more information.  
   使用 `menuconfig` 调整项目设置，例如 `CONFIG_SERVER_PORT`、Wi-Fi SSID/密码、LED 管脚配置及其它相关设置。更多信息请参阅 `Kconfig.projbuild`。

2. **Build the Project:**  
   Run `idf.py build` to compile the project.  
   执行 `idf.py build` 进行编译。

3. **Flash the Firmware:**  
   Connect your ESP32 board and run `idf.py flash` to upload the firmware.  
   将 ESP32 开发板连接后，执行 `idf.py flash` 烧录固件。

4. **Monitor Output:**  
   Use `idf.py monitor` to view the debug output and verify that the system is working as expected.  
   使用 `idf.py monitor` 查看调试输出，验证系统正常工作。

## Handling Wi-Fi Disconnection / Wi-Fi 断连处理

- The Wi-Fi event handler automatically attempts to reconnect if disconnected.  
  Wi-Fi 事件处理程序在断连时自动尝试重连。

- Dependent tasks (TCP server, Process_Data, etc.) are only started when Wi-Fi is connected.  
  依赖 Wi-Fi 的任务（TCP 服务器、Process_Data 等）只在 Wi-Fi 连接成功后启动。

- Additional logic can be implemented to pause or buffer data if the Wi-Fi connection is lost.  
  如果 Wi-Fi 断连，你可以加入额外逻辑暂停或缓冲数据。

## Notes / 注意事项

- When sending binary data (e.g., the Command structure), ensure that both sender and receiver use the same structure definition (consider using packed structures).  
  发送二进制数据（如 Command 结构体）时，请确保发送端和接收端使用相同的结构体定义（建议使用 packed 结构）。

- The project relies on FreeRTOS event groups and semaphores to synchronize tasks and protect shared resources (e.g., the broadcast JSON string).  
  本项目依赖 FreeRTOS 事件组和信号量来同步任务及保护共享资源（例如广播 JSON 字符串）。

- Modify the timeout values and error handling as needed for your application requirements.  
  根据实际需求调整超时值和错误处理逻辑。

- **LED Indicator:**  
  An LED is used to indicate system status such as Wi-Fi connection and error conditions. You can adjust the LED pin and behavior via menuconfig. Refer to `Kconfig.projbuild` for more details.  
  LED 指示灯用于显示系统状态（如 Wi-Fi 连接状态和错误情况）。你可以通过 menuconfig 调整 LED 管脚及行为，详情请参阅 `Kconfig.projbuild`。

---
