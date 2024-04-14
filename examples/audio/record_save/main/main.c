#include <stdio.h>
#include "hal_sd.h"
#include "hal_i2s.h"

hal_sd_spi_config_t hal_sd_config = {
    .clk_pin = GPIO_NUM_39,
    .cmd_pin = GPIO_NUM_38,
    .d0_pin = GPIO_NUM_40,
    .mount_path = "/sdcard",
};

i2s_microphone_config_t i2s_microphone_config = {
    .bclk_pin = GPIO_NUM_41,
    .ws_pin = GPIO_NUM_42,
    .din_pin = GPIO_NUM_2,
    .i2s_num = I2S_NUM_1,
    .sample_rate = 16 * 1000,
    .bits_per_sample = I2S_DATA_BIT_WIDTH_16BIT,
};

void app_main(void)
{
    ESP_ERROR_CHECK(hal_sd_init(hal_sd_config));
    ESP_ERROR_CHECK(hal_i2s_microphone_init(i2s_microphone_config));

    hal_i2s_record("/sdcard/record.wav", 10);
}
