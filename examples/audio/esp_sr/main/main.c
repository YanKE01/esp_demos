#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "model_path.h"
#include "esp_process_sdkconfig.h"
#include "driver/i2s_common.h"
#include "driver/i2s_std.h"
#include "led_strip.h"
#include "esp_mn_speech_commands.h"
#include "hal_i2s.h"
#include "app_sr.h"
#include "app_led.h"


i2s_config_t i2s_config = {
    .bclk_pin = GPIO_NUM_41,
    .ws_pin = GPIO_NUM_42,
    .din_pin = GPIO_NUM_2,
    .i2s_num = I2S_NUM_1,
    .sample_rate = 16000,
    .bits_per_sample = 32,
};

void app_main(void)
{
    app_led_init(GPIO_NUM_38, 8);
    ESP_ERROR_CHECK(hal_i2s_init(i2s_config));
    ESP_ERROR_CHECK(app_sr_init("model"));
    app_sr_task_start();
}
