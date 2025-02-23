#include <stdio.h>

#include <lwip/err.h>    
#include <lwip/netdb.h>   
#include <lwip/sockets.h> 
#include <lwip/sys.h>      

#include "cJSON.h"        

#include "esp_event.h"
#include "esp_log.h"     
#include "esp_system.h"    
#include "esp_wifi.h"      

#include "freertos/FreeRTOS.h" 
#include "freertos/queue.h"    
#include "freertos/task.h"    

#include "TCPServer.h"    
#include "user_uart.h"

static int client_socks[3] = { -1,-1,-1 };
static SemaphoreHandle_t client_mutex = NULL;

static uint8_t s_retry_num = 0;
static EventGroupHandle_t s_wifi_event_group; /* FreeRTOS event group to signal when connected*/
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define TCP_INIT_BIT BIT2

#define Target_WiFi_1_SSID ("BananaPiWifi")
#define Target_WiFi_1_PASSWORD ("BananaPiWifi")
#define Target_WiFi_2_SSID ("WPS School")
#define Target_WiFi_2_PASSWORD ("WPS School")

#define KEEPALIVE_IDLE 5
#define KEEPALIVE_INTERVAL 5
#define KEEPALIVE_COUNT 3

/**
 * @brief Handle WiFi events
 * @param arg User-defined argument
 * @param event_base Event base
 * @param event_id Event ID
 * @param event_data Event data
 * @retval None
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < CONFIG_WIFI_MAX_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI("WiFi", "retry to connect to the AP");
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI("WiFi", "connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        ESP_LOGI("WiFi", "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        EventBits_t bits = xEventGroupGetBits(s_wifi_event_group);
        if (!(bits & TCP_INIT_BIT)) Init_TCPServer();
    }
}

/**
 * @brief Parse command from a message string
 * @param msg Message string
 * @retval Parsed command
 */
Command parse_command(const char* msg)
{
    Command cmd;
    cmd.type = CMD_UNKNOWN;
    memset(&cmd.params, 0, sizeof(cmd.params));

    // Copy the input string to a temporary buffer because strtok modifies the string
    // 复制传入的字符串到临时缓冲区，因为strtok会修改字符串
    char* msg_copy = strdup(msg);
    if (msg_copy == NULL)
    {
        return cmd;
    }

    // Split the string by spaces and get the first token as the command type
    // 用空格拆分字符串，获取第一个token作为命令类型
    char* token = strtok(msg_copy, " ");
    if (token == NULL)
    {
        free(msg_copy);
        return cmd;
    }

    if (strcmp(token, "move") == 0)
    {
        cmd.type = CMD_MOVE;
        // 解析格式: move [Stop] [S/D] [WASD] [Value] [Time]
        token = strtok(NULL, " ");
        if (token != NULL)
            cmd.params.move.stop = atoi(token);
        token = strtok(NULL, " ");
        if (token != NULL)
            cmd.params.move.sd = token[0];
        token = strtok(NULL, " ");
        if (token != NULL)
            strncpy(cmd.params.move.wasd, token, sizeof(cmd.params.move.wasd) - 1);
        token = strtok(NULL, " ");
        if (token != NULL)
            cmd.params.move.value = atoi(token);
        token = strtok(NULL, " ");
        if (token != NULL)
            cmd.params.move.time = atoi(token);
    }
    else if (strcmp(token, "spin") == 0)
    {
        cmd.type = CMD_SPIN;
        // 解析格式: spin [L/R] [Angle]
        token = strtok(NULL, " ");
        if (token != NULL)
            cmd.params.spin.lr = token[0];
        token = strtok(NULL, " ");
        if (token != NULL)
            cmd.params.spin.angle = atoi(token);
    }
    else if (strcmp(token, "motor") == 0)
    {
        cmd.type = CMD_MOTOR;
        // 解析格式: motor [MotorID] [Dir] [Angle]
        token = strtok(NULL, " ");
        if (token != NULL)
            cmd.params.motor.motorID = atoi(token);
        token = strtok(NULL, " ");
        if (token != NULL)
            cmd.params.motor.dir = token[0];
        token = strtok(NULL, " ");
        if (token != NULL)
            cmd.params.motor.angle = atoi(token);
    }
    else
    {
        cmd.type = CMD_UNKNOWN;
    }

    free(msg_copy);
    return cmd;
}

/**
 * @brief Process client data received in JSON format
 * @param json_input JSON input string
 * @retval None
 */
void Process_Client_Data(const char* json_input)
{
    cJSON* root = cJSON_Parse(json_input);
    if (root == NULL)
    {
        ESP_LOGE("TCP_Server", "Invalid JSON input");
        return;
    }

    // Get the "Msg" field
    // 获取 "Msg" 字段
    cJSON* msg_item = cJSON_GetObjectItem(root, "Msg");
    if (msg_item == NULL || msg_item->valuestring == NULL)
    {
        ESP_LOGE("TCP_Server", "No Msg field found");
        cJSON_Delete(root);
        return;
    }

    // Call parse_command to parse the command string
    // 调用parse_command解析指令字符串
    Command cmd = parse_command(msg_item->valuestring);
    if (cmd.type != CMD_UNKNOWN) uart_send((const char*)&cmd, sizeof(cmd));

    cJSON_Delete(root);
}

/**
 * @brief Task to handle client connections
 * @param pvParameters Task parameters
 * @retval None
 */
void handle_client_task(void* pvParameters)
{
    int sock = (int)pvParameters;
    int len;
    char rx_buffer[256];

    while (1)
    {
        len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0)
        {
            ESP_LOGE("TCP_Server", "recv error: errno %d", errno);
            break;
        }
        else if (len == 0)
        {
            ESP_LOGW("TCP_Server", "Connection closed");
            break;
        }
        else
        {
            rx_buffer[len] = 0;
            ESP_LOGI("TCP_Server", "Received %d bytes from client: %s", len, rx_buffer);
            Process_Client_Data(rx_buffer);
        }
    }

    // Exit
    xSemaphoreTake(client_mutex, portMAX_DELAY);
    for (uint8_t i = 0;i < 3;i++)
        if (client_socks[i] == sock)
        {
            client_socks[i] = -1;
            break;
        }
    shutdown(sock, 0);
    close(sock);
    ESP_LOGI("TCP_Server", "Client disconnected, task deleted");
    vTaskDelete(NULL);
}

/**
 * @brief Initialize WiFi
 * @retval None
 */
void Init_WiFi(void)
{
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_config_init = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_config_init));
    esp_wifi_set_ps(WIFI_PS_NONE);

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &wifi_event_handler,
        NULL,
        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &wifi_event_handler,
        NULL,
        &instance_got_ip));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI("WiFi", "WiFi initialization finished, scanning for target APs...");

    uint16_t Scan_List_Num = CONFIG_WIFI_SCAN_LIST_SIZE;
    wifi_ap_record_t ap_info[CONFIG_WIFI_SCAN_LIST_SIZE];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));

    esp_wifi_scan_start(NULL, true);

    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&Scan_List_Num, ap_info));
    if (ap_count == 0)
    {
        ESP_LOGI("WiFi", "No AP found during scan");
        return;
    }

    bool Found_WiFi1 = false, Found_WiFi2 = false;
    int8_t Best_rssi = -127;
    uint16_t Best_rssi_Index = 0;
    for (uint16_t i = 0; i < Scan_List_Num; i++)
    {
        if (strcmp((char*)ap_info[i].ssid, Target_WiFi_1_SSID) == 0)
        {
            Found_WiFi1 = true;
            if (ap_info[i].rssi > Best_rssi)
            {
                Best_rssi = ap_info[i].rssi;
                Best_rssi_Index = i;
            }
        }
    }
    if (Found_WiFi1 != true)
    {
        for (uint8_t i = 0; i < Scan_List_Num; i++)
        {
            if (strcmp((char*)ap_info[i].ssid, Target_WiFi_2_SSID) == 0)
            {
                Found_WiFi2 = true;
                if (ap_info[i].rssi > Best_rssi)
                {
                    Best_rssi = ap_info[i].rssi;
                    Best_rssi_Index = i;
                }
            }
        }
    }
    if ((Found_WiFi1 != true) && (Found_WiFi2 != true))
    {
        ESP_LOGI("WiFi", "Both WiFis were not found");
        return;
    }
    else
    {
        ESP_LOGI("WiFi", "Best AP found: %s, RSSI: %d", ap_info[Best_rssi_Index].ssid, ap_info[Best_rssi_Index].rssi);
    }

    wifi_config_t wifi_config = { 0 };
    strncpy((char*)wifi_config.sta.ssid, (const char*)ap_info[Best_rssi_Index].ssid, sizeof(wifi_config.sta.ssid) - 1);
    memcpy(wifi_config.sta.bssid, ap_info[Best_rssi_Index].bssid, sizeof(wifi_config.sta.bssid));
    strncpy((char*)wifi_config.sta.password, (strcmp((char*)ap_info[Best_rssi_Index].ssid, Target_WiFi_1_SSID) == 0) ? Target_WiFi_1_PASSWORD : Target_WiFi_2_PASSWORD, sizeof(wifi_config.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI("WiFi", "connected to ap SSID:%s", ap_info[Best_rssi_Index].ssid);
        Init_TCPServer();
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI("WiFi", "Failed to connect to SSID:%s", ap_info[Best_rssi_Index].ssid);
        ESP_LOGW("WiFi", "TCP Server not initialed due to WiFi not ready");
    }
}

/**
 * @brief Initialize TCP server
 * @retval None
 */
void Init_TCPServer(void)
{
    char addr_str[128];
    int addr_family = AF_INET;
    int ip_protocol = IPPROTO_IP;
    int keepAlive = 1;
    int keepIdle = KEEPALIVE_IDLE;
    int keepInterval = KEEPALIVE_INTERVAL;
    int keepCount = KEEPALIVE_COUNT;
    struct sockaddr_in dest_addr = {
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_family = AF_INET,
        .sin_port = htons(CONFIG_SERVER_PORT),
    };
    ESP_LOGI("TCP_Server", "Initializing socket...");
    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0)
    {
        ESP_LOGE("TCP_Server", "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }

    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    ESP_LOGI("TCP_Server", "Socket created");

    ESP_LOGI("TCP_Server", "Binding socket...");
    int err = bind(listen_sock, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    if (err != 0)
    {
        ESP_LOGE("TCP_Server", "Socket unable to bind: errno %d", errno);
        goto CLEAN_UP;
    }
    ESP_LOGI("TCP_Server", "Socket bound, port %d", CONFIG_SERVER_PORT);

    ESP_LOGI("TCP_Server", "Listening");
    err = listen(listen_sock, 3);
    if (err != 0)
    {
        ESP_LOGE("TCP_Server", "Error during listen: errno %d", errno);
        goto CLEAN_UP;
    }
    ESP_LOGI("TCP_Server", "Socket listening");
    client_mutex = xSemaphoreCreateMutex();

    xEventGroupSetBits(s_wifi_event_group, TCP_INIT_BIT);

    ESP_LOGI("TCP_Server", "Waiting for client connections...");
    while (1)
    {
        struct sockaddr_in source_addr;
        socklen_t addr_len = sizeof(source_addr);

        int sock = accept(listen_sock, (struct sockaddr*)&source_addr, &addr_len);
        if (sock < 0)
        {
            ESP_LOGE("TCP_Server", "Unable to accept connection: errno %d", errno);
            break;
        }
        inet_ntoa_r(source_addr.sin_addr, addr_str, sizeof(addr_str) - 1);
        ESP_LOGI("TCP_Server", "Socket accepted, IP address: %s", addr_str);

        setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));

        bool ClientAdded = false;
        xSemaphoreTake(client_mutex, portMAX_DELAY);
        for (uint8_t i = 0;i < 3;i++)
            if (client_socks[i] < 0)
            {
                client_socks[i] = sock;
                ClientAdded = true;
                xSemaphoreGive(client_mutex);
                break;
            }
        if (ClientAdded) xTaskCreate(handle_client_task, "handle_client_task", 4096, (void*)sock, 5, NULL);
        else
        {
            ESP_LOGW("TCP_Server", "Client list full, dropping new client");
            shutdown(sock, 0);
            close(sock);
        }
    }
CLEAN_UP:
    close(listen_sock);
    vTaskDelete(NULL);
}

/**
 * @brief Task to process data and send it to clients
 * @param pvParameters Task parameters
 * @retval None
 */
void Process_Data(void* pvParameters)
{
    while (1)
    {
        SensorData_t* pData = NULL;
        if (xQueueReceive(uart_queue, &pData, portMAX_DELAY) == pdPASS)
        {
            EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                pdFALSE,
                pdFALSE,
                pdMS_TO_TICKS(200));

            if (bits & WIFI_CONNECTED_BIT)
            {
                cJSON* root = cJSON_CreateObject();
                if (root)
                {
                    cJSON_AddStringToObject(root, "type", "data");
                    cJSON* data_obj = cJSON_CreateObject();
                    if (data_obj)
                    {
                        int wifi_rssi = -127;
                        esp_wifi_sta_get_rssi(&wifi_rssi);
                        cJSON_AddNumberToObject(data_obj, "WifiSignalStrength", wifi_rssi);
                        cJSON_AddNumberToObject(data_obj, "Voltage", pData->Voltage);
                        cJSON_AddNumberToObject(data_obj, "Temperature", pData->Temperature);

                        cJSON* imu = cJSON_CreateObject();
                        if (imu)
                        {
                            cJSON_AddNumberToObject(imu, "accel_x", pData->IMUData.accel_x);
                            cJSON_AddNumberToObject(imu, "accel_y", pData->IMUData.accel_y);
                            cJSON_AddNumberToObject(imu, "accel_z", pData->IMUData.accel_z);
                            cJSON_AddNumberToObject(imu, "gyro_x", pData->IMUData.gyro_x);
                            cJSON_AddNumberToObject(imu, "gyro_y", pData->IMUData.gyro_y);
                            cJSON_AddNumberToObject(imu, "gyro_z", pData->IMUData.gyro_z);
                            cJSON_AddNumberToObject(imu, "Temperature", pData->IMUData.Temperature);
                            cJSON_AddItemToObject(data_obj, "IMUData", imu);
                        }

                        cJSON* motors = cJSON_CreateArray();
                        if (motors)
                        {
                            for (int i = 0; i < CONFIG_MOTOR_COUNT; i++)
                            {
                                cJSON* motor = cJSON_CreateObject();
                                if (motor)
                                {
                                    cJSON_AddNumberToObject(motor, "Speed", pData->Motor[i].Speed);

                                    const char* dirStr = (pData->Motor[i].Direction == CW) ? "CW" : "CCW";
                                    cJSON_AddStringToObject(motor, "Direction", dirStr);
                                    cJSON_AddNumberToObject(motor, "Amps", pData->Motor[i].Amps);
                                    cJSON_AddItemToArray(motors, motor);
                                }
                            }
                            cJSON_AddItemToObject(data_obj, "Motor", motors);
                        }
                    }
                    cJSON_AddItemToObject(root, "data", data_obj);

                    char* json_str = cJSON_PrintUnformatted(root);
                    if (json_str)
                    {
                        xSemaphoreTake(client_mutex, portMAX_DELAY);
                        for (uint8_t i = 0;i < 3;i++)
                            if (client_socks[i] >= 0)
                            {
                                int sent = send(client_socks[i], json_str, strlen(json_str), 0);
                                if (sent < 0) ESP_LOGE("TCP_Server", "Error sending to client %d: errno %d", client_socks[i], errno);
                            }
                        xSemaphoreGive(client_mutex);
                        free(json_str);
                    }
                    cJSON_Delete(root);
                }
            }
            else
            {
                ESP_LOGE("TCP_Server", "WiFi not ready, skipping broadcast");
            }
            free(pData);
        }
    }
    vTaskDelete(NULL);
}