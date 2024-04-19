#include <stdio.h>
#include "ssd1306.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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
    int x_satrt = -100;
    ESP_ERROR_CHECK(ssd1306_init(ssd1306_hal_config));
    window_t win = {x_satrt, 0, 128, 64};
    while (1)
    {
        oled_clear();
        if (x_satrt < 0)
        {
            x_satrt++;
        }
        win.start_x = x_satrt;
        oled_win_draw_str(&win, 25, 5, "hello world");
        oled_refersh();
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}
