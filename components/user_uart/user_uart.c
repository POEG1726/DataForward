#include <stdio.h>
#include <string.h>
#include "user_uart.h"
#include "TCPServer.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"

#define BUFFER_SIZE (256)

QueueHandle_t uart_queue;

void uart_receive_task(void *pvParameters)
{
    uint8_t buffer[BUFFER_SIZE];
    while (1)
    {
        int len = uart_read_bytes(UART_NUM_1, buffer, BUFFER_SIZE, pdMS_TO_TICKS(1000));
        if (len > 0)
        {
            uint8_t *tmp_buf=malloc(len);
            memcpy(tmp_buf, buffer, len);
            if (xQueueSend(uart_queue, &tmp_buf, pdMS_TO_TICKS(10)) != pdPASS) {
                ESP_LOGW("UART", "Queue full, dropping sensor data");
                free(tmp_buf);
            }
        }
    }
    vTaskDelete(NULL);
}

void Init_uart(void)
{
    uart_config_t config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .parity = UART_PARITY_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
        .stop_bits = 1,
    };

    uart_driver_install(UART_NUM_1, BUFFER_SIZE, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &config);
    uart_set_pin(UART_NUM_1, CONFIG_UART_TX_PIN, CONFIG_UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    uart_queue = xQueueCreate(5, sizeof(uint8_t *));
    xTaskCreate(uart_receive_task, "uart receive task", 4096, NULL, 10, NULL);
}

void uart_send(const char *msg)
{
    const uint16_t length = strlen(msg);
    uart_write_bytes(UART_NUM_1, msg, length);
}