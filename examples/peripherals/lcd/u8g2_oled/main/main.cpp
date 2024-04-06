#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "u8g2_port.h"

extern "C"
{
#include "u8g2_esp32_hal.h"
}

extern "C" void app_main(void)
{
    u8g2_esp32_hal_t u8g2_esp32_hal = {
        {GPIO_NUM_5, GPIO_NUM_2, GPIO_NUM_35}, // clk,mosi,cs
        GPIO_NUM_7,                            // reset
        GPIO_NUM_1,                            // dc
    };
    u8g2_esp32_hal_init(u8g2_esp32_hal);
    U8G2_SSD1306_128X64_ESP_SPI u8g2;

    u8g2.begin();
    u8g2.clearBuffer();
    u8g2.enableUTF8Print();
    u8g2.setFont(u8g2_font_HelvetiPixel_tr);
    u8g2.setCursor(0, 20);
    u8g2.print("hello esp32");
    u8g2.sendBuffer();
}
