#include <stdio.h>
#include "LED.h"
#include "led_strip.h"
#include "driver/gpio.h"

#define LED_STRIP_RMT_RES_HZ (10 * 1000 * 1000)

static led_strip_handle_t led_strip;

void LED_Init(void)
{
    gpio_config_t LED_config = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1 << CONFIG_LED1_PIN),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    gpio_config(&LED_config);

    led_strip_config_t strip_config = {
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_RGB,
        .flags = {0},
        .led_model = LED_MODEL_WS2812,
        .max_leds = 1,
        .strip_gpio_num = CONFIG_WS2812_PIN,
    };
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .flags = {
            .with_dma = 0,
        },
        .mem_block_symbols = 8,
        .resolution_hz = LED_STRIP_RMT_RES_HZ,
    };
    led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);
}

void LED_Switch(uint8_t status)
{
    gpio_set_level(CONFIG_LED1_PIN, status);
}

void ws2812(uint8_t Status, uint32_t color_R, uint32_t color_G, uint32_t color_B)
{
    switch (Status)
    {
    case 0:
        led_strip_clear(led_strip);
        // led_strip_del(led_strip);
        break;
    case 1:
        led_strip_set_pixel(led_strip, 0, color_R, color_G, color_B);
        led_strip_refresh(led_strip);
        break;
    }
}