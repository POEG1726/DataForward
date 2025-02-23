#include <stdio.h>
#include "TCPServer.h"
#include "user_uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_system.h"
#include "cJSON.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include <lwip/netdb.h>

static uint8_t s_retry_num = 0;
/* FreeRTOS event group to signal when connected*/
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

#define Target_WiFi_1_SSID ("BananaPiWifi")
#define Target_WiFi_1_PASSWORD ("BananaPiWifi")
#define Target_WiFi_2_SSID ("WPS School")
#define Target_WiFi_2_PASSWORD ("WPS School")

#define KEEPALIVE_IDLE 5
#define KEEPALIVE_INTERVAL 5
#define KEEPALIVE_COUNT 3

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
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
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI("WiFi", "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void Init_WiFi(void)
{
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_config_init = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_config_init));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
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
        if (strcmp((char *)ap_info[i].ssid, Target_WiFi_1_SSID) == 0)
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
            if (strcmp((char *)ap_info[i].ssid, Target_WiFi_2_SSID) == 0)
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

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, (const char *)ap_info[Best_rssi_Index].ssid, sizeof(wifi_config.sta.ssid) - 1);
    memcpy(wifi_config.sta.bssid, ap_info[Best_rssi_Index].bssid, sizeof(wifi_config.sta.bssid));
    strncpy((char *)wifi_config.sta.password, (strcmp((char *)ap_info[Best_rssi_Index].ssid, Target_WiFi_1_SSID) == 0) ? Target_WiFi_1_PASSWORD : Target_WiFi_2_PASSWORD, sizeof(wifi_config.sta.password) - 1);

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
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI("WiFi", "Failed to connect to SSID:%s", ap_info[Best_rssi_Index].ssid);
    }
}

void Init_TCPServer(void)
{
    char addr_str[128];
    int addr_family = AF_INET;
    int ip_protocol = IPPROTO_IP;
    int keepAlive = 1;
    int keepIdle = KEEPALIVE_IDLE;
    int keepInterval = KEEPALIVE_INTERVAL;
    int keepCount = KEEPALIVE_COUNT;
    struct sockaddr_in *dest_addr = {dsa
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_family = AF_INET,
        .sin_port = htons(CONFIG_SERVER_PORT),
    };

    ESP_LOGI("TCP_Server", "1. Initializing socket...");
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

    ESP_LOGI("TCP_Server", "2. Binding socket...");
    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0)
    {
        ESP_LOGE("TCP_Server", "Socket unable to bind: errno %d", errno);
        close(listen_sock);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI("TCP_Server", "Socket bound, port %d", PORT);

    ESP_LOGI("TCP_Server", "3. Listening...");
    // 设置 backlog 为 3，支持同时接受 3 个客户端连接
    err = listen(listen_sock, 3);
    if (err != 0)
    {
        ESP_LOGE("TCP_Server", "Error during listen: errno %d", errno);
        close(listen_sock);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI("TCP_Server", "Socket listening");

    ESP_LOGI("TCP_Server", "4. Waiting for client connections...");
    while (1)
    {
        struct sockaddr_in source_addr;
        socklen_t addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0)
        {
            ESP_LOGE("TCP_Server", "Unable to accept connection: errno %d", errno);
            break;
        }

        // 将客户端IP地址转换为字符串
        inet_ntoa_r(source_addr.sin_addr, addr_str, sizeof(addr_str) - 1);
        ESP_LOGI("TCP_Server", "Socket accepted, IP address: %s", addr_str);

        // 设置TCP keepalive选项
        setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &KEEPALIVE_IDLE, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &KEEPALIVE_INTERVAL, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &KEEPALIVE_COUNT, sizeof(int));

        // 为每个客户端创建一个任务进行处理
        xTaskCreate(handle_client_task, "handle_client_task", 4096, (void *)sock, 5, NULL);
    }

    close(listen_sock);
    vTaskDelete(NULL);
}
}

void Process_Data(void *pvParameters)
{
    while (1)
    {
        SensorData_t *pData = NULL;
        if (xQueueReceive(uart_queue, &pData, portMAX_DELAY) == pdPASS)
        {
            cJSON *root = cJSON_CreateObject();
            if (root)
            {
                int wifi_rssi = -127;
                esp_wifi_sta_get_rssi(&wifi_rssi);
                cJSON_AddNumberToObject(root, "WifiSignalStrength", wifi_rssi);
                cJSON_AddNumberToObject(root, "Voltage", pData->Voltage);
                cJSON_AddNumberToObject(root, "Temperature", pData->Temperature);

                cJSON *imu = cJSON_CreateObject();
                if (imu)
                {
                    cJSON_AddNumberToObject(imu, "accel_x", pData->IMUData.accel_x);
                    cJSON_AddNumberToObject(imu, "accel_y", pData->IMUData.accel_y);
                    cJSON_AddNumberToObject(imu, "accel_z", pData->IMUData.accel_z);
                    cJSON_AddNumberToObject(imu, "gyro_x", pData->IMUData.gyro_x);
                    cJSON_AddNumberToObject(imu, "gyro_y", pData->IMUData.gyro_y);
                    cJSON_AddNumberToObject(imu, "gyro_z", pData->IMUData.gyro_z);
                    cJSON_AddNumberToObject(imu, "Temperature", pData->IMUData.Temperature);
                    cJSON_AddItemToObject(root, "IMUData", imu);
                }

                cJSON *motors = cJSON_CreateArray();
                if (motors)
                {
                    for (int i = 0; i < CONFIG_MOTOR_COUNT; i++)
                    {
                        cJSON *motor = cJSON_CreateObject();
                        if (motor)
                        {
                            cJSON_AddNumberToObject(motor, "Speed", pData->Motor[i].Speed);
                            // 将枚举转成字符串
                            const char *dirStr = (pData->Motor[i].Direction == CW) ? "CW" : "CCW";
                            cJSON_AddStringToObject(motor, "Direction", dirStr);
                            cJSON_AddNumberToObject(motor, "Amps", pData->Motor[i].Amps);
                            cJSON_AddItemToArray(motors, motor);
                        }
                    }
                    cJSON_AddItemToObject(root, "Motor", motors);
                }
                char *json_str = cJSON_PrintUnformatted(root);
                if (json_str)
                {
                    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                                           pdFALSE,
                                                           pdFALSE,
                                                           portMAX_DELAY);

                    if (bits & WIFI_CONNECTED_BIT)
                    {
                        // Send to TCP
                    }
                    free(json_str);
                }
                cJSON_Delete(root);
            }
            free(pData);
        }
    }
    vTaskDelete(NULL);
}