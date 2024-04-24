#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_log.h"
#include "ui.h"
#include "iot_button.h"

ssd1306_hal_config_t ssd1306_hal_config = {
    .dc = GPIO_NUM_42,
    .rst = GPIO_NUM_6,
    .bus.spi = {
        .clk = GPIO_NUM_5,
        .cs = GPIO_NUM_41,
        .mosi = GPIO_NUM_2,
        .spi_host_num = SPI2_HOST,
    },
};

list_elemt_t main_list[] = {
    {"[ Main ]", NO_ACTION},
    {"- Setting", ENTER_NEXT_PAGE},
    {"- Wave", POP_WINDOW_SHOW},
    {"~ Io Test", POP_WINDOW_SHOW},
    {"- About", POP_WINDOW_SHOW},
};

list_elemt_t setting_list[] = {
    {"[ Setting ]", BACK_PREV_PAGE},
    {"~ Kp", POP_WINDOW_SHOW},
    {"~ Ki", POP_WINDOW_SHOW},
    {"~ Kd", POP_WINDOW_SHOW},
    {"~ Led R", POP_WINDOW_SHOW},
    {"~ Led G", POP_WINDOW_SHOW},
    {"~ Led B", POP_WINDOW_SHOW},
};

button_handle_t btn_handle;

void app_main(void)
{
    ESP_ERROR_CHECK(ssd1306_init(ssd1306_hal_config));

    button_config_t btn_cfg = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = 1000,
        .short_press_time = 100,
        .gpio_button_config = {
            .gpio_num = 0,
            .active_level = 0,
        },
    };

    btn_handle = iot_button_create(&btn_cfg);
    iot_button_register_cb(btn_handle, BUTTON_SINGLE_CLICK, input_set_scroll_down, NULL);
    iot_button_register_cb(btn_handle, BUTTON_DOUBLE_CLICK, input_set_enter, NULL);

    ui_page_add_list(0, main_list, sizeof(main_list) / sizeof(main_list[0]));
    ui_page_add_list(1, setting_list, sizeof(setting_list) / sizeof(setting_list[0]));

    while (1)
    {
        ui_proc();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
