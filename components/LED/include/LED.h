// COLOR_RED        (255, 0, 0)
// COLOR_GREEN      (0, 255, 0)
// COLOR_BLUE       (0, 0, 255)
// COLOR_YELLOW     (255, 255, 0)
// COLOR_ORANGE     (255, 165, 0)
// COLOR_PURPLE     (128, 0, 128)
// COLOR_WHITE      (255, 255, 255)
// COLOR_NONE       (0, 0, 0)

void LED_Init(void);
void LED_Switch(uint8_t status);
void ws2812(uint8_t Status, uint32_t color_R, uint32_t color_G, uint32_t color_B);