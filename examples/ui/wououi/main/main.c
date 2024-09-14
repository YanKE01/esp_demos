#include <stdio.h>
#include "ssd1306.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ui.h"
#include "iot_button.h"
#include "esp_log.h"

button_handle_t btn_handle;
static const char *TAG = "MAIN";

void main_menu_cb();
void setting_menu_cb();

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

list_page_t main_menu_list_page = {
    .ui_state = MENU,
    .list_state = LIST,
    .list_element = {"[ Main ]", "- Setting", "- Editor", "- Wave", "- IO Test", "- About"},
    .index = 0,
    .win = {0, 0, 128, 64},
    .cb = main_menu_cb,
};

list_page_t setting_menu_list_page = {
    .ui_state = MENU,
    .list_state = LIST,
    .list_element = {"[ Setting ]", "~ Disp Bri", "~ X", "~ Y", "~ Z"},
    .index = 1,
    .win = {0, 0, 128, 64},
    .cb = setting_menu_cb,
};

list_page_t setting_bright_win_page = {
    .ui_state = MENU,
    .list_state = WIN,
    .index = 2,
    .win_page.title = "Disp Bri",
    .win_page.max = 100,
    .win_page.min = 10,
    .win_page.value = 50,
    .win = {0, 0, 128, 64},
};

void main_menu_cb()
{
    if (list_page[list_page_index].select == 1) {
        // 选中setting
        ESP_LOGI(TAG, "Setting");
        jump_page(1); // 跳转到Siettng
    }
}

void setting_menu_cb()
{
    if (list_page[list_page_index].select == 0) {
        // 选中顶部
        ESP_LOGI(TAG, "Back");
        jump_page(0); // 跳转到main
    } else if (list_page[list_page_index].select == 1) {
        // 设置窗口页面
        jump_page(2);
    }
}




void ui_task(void *args)
{
    while (1) {
        ui_proc();
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}

void short_down(void *button_handle, void *usr_data)
{
    input_roll_cb();
    ESP_LOGI(TAG, "Short down");
}

void double_down(void *button_handle, void *usr_data)
{
    input_enter_cb();
    ESP_LOGI(TAG, "doule down");
}

void app_main()
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
    iot_button_register_cb(btn_handle, BUTTON_SINGLE_CLICK, short_down, NULL);
    iot_button_register_cb(btn_handle, BUTTON_DOUBLE_CLICK, double_down, NULL);

    ui_add_page(main_menu_list_page);
    ui_add_page(setting_menu_list_page);
    ui_add_page(setting_bright_win_page);
    xTaskCreate(ui_task, "ui", 4 * 1024, NULL, 5, NULL);
}
