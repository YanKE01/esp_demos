#include <stdlib.h>
#include "stdio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "nvs_flash.h"
#include "string.h"
#include "app_wifi.h"
#include "app_http_tts.h"
#include "hal_i2s.h"
#include "app_spiffs.h"
#include "app_http_asr.h"
#include "app_http_tongyi.h"
#include "iot_button.h"

i2s_microphone_config_t i2s_microphone_config = {
    .bclk_pin = GPIO_NUM_41,
    .ws_pin = GPIO_NUM_42,
    .din_pin = GPIO_NUM_2,
    .i2s_num = I2S_NUM_1,
    .sample_rate = 16 * 1000,
    .bits_per_sample = I2S_DATA_BIT_WIDTH_16BIT,
};

i2s_speaker_config_t i2s_speaker_config = {
    .bclk_pin = 10,
    .dout_pin = 11,
    .ws_pin = 9,
    .i2s_num = I2S_NUM_0,
    .bits_per_sample = I2S_DATA_BIT_WIDTH_16BIT,
    .sample_rate = 16 * 1000,
};

button_handle_t btn_handle;

void btn_press_cb(void *button_handle, void *usr_data)
{
    app_http_asr(3);
}

void app_main(void)
{
    // Init NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Init spiffs
    ESP_ERROR_CHECK(app_spiffs_init("/spiffs"));

    // Init I2S
    ESP_ERROR_CHECK(hal_i2s_speaker_init(i2s_speaker_config));
    ESP_ERROR_CHECK(hal_i2s_microphone_init(i2s_microphone_config));

    // Init wifi
    app_wifi_init("ChinaUnicom-3LRNAS", "244244244");

    // Init button
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
    iot_button_register_cb(btn_handle, BUTTON_SINGLE_CLICK, btn_press_cb, NULL);

    xTaskCreatePinnedToCore(app_http_ask_tongyi_task, "Tongyi", 4 * 2048, NULL, 5, NULL, 0);

}
