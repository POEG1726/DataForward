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
    // ws2812(1, 255, 255, 255);
    /*
    wifi扫描以及连接阶段:ws2812蓝色呼吸灯
    wifi连接成功:ws2812蓝色闪烁3次
    wifi连接失败:ws2812红色常亮
    wifi连接成功到tcp初始化:ws2812蓝色到绿色渐变,循环
    wifi以及tcp成功建立:绿色常亮

    uart错误:2个led常亮
    uart发送中:绿色led闪烁
    uart接受中:蓝色led闪烁
    */
    Init_uart();
    Init_WiFi();
}
