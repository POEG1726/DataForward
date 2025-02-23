#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "nvs_flash.h"

#include "LED.h"
#include "TCPServer.h"
#include "user_uart.h"

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    LED_Init();
    Init_uart();
    Init_WiFi();
}
