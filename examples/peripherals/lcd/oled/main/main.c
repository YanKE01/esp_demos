#include <stdio.h>
#include "ssd1306.h"
#include "driver/gpio.h"

ssd1306_hal_config_t ssd1306_hal_config = {
    .dc = GPIO_NUM_1,
    .rst = GPIO_NUM_7,
    .bus.spi = {
        .clk = GPIO_NUM_5,
        .cs = GPIO_NUM_35,
        .mosi = GPIO_NUM_2,
        .spi_host_num = SPI2_HOST,
    },
};

void app_main(void)
{
    ESP_ERROR_CHECK(ssd1306_init(ssd1306_hal_config));
    oled_write_byte(64, 0, 1, NORMAL);
    oled_write_byte(64, 1, 1, NORMAL);
    oled_refersh();
}
