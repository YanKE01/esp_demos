#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal_i2s.h"
#include "freertos/ringbuf.h"

#define BYTE_RATE 16 * 1000 * (I2S_DATA_BIT_WIDTH_16BIT / 8) * 1 // 我们是I2S_DATA_BIT_WIDTH_16BIT，所以占2个字节
int rec_time = 5;
int32_t i2s_readraw_buff[I2S_DATA_BIT_WIDTH_16BIT * 1024];
size_t bytes_read = 0;
size_t bytes_write = 0;

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

void app_main(void)
{

    ESP_ERROR_CHECK(hal_i2s_microphone_init(i2s_microphone_config));
    ESP_ERROR_CHECK(hal_i2s_speaker_init(i2s_speaker_config));

    while (1) {
        if (i2s_channel_read(rx_handle, (char *)i2s_readraw_buff, I2S_DATA_BIT_WIDTH_16BIT * 1024, &bytes_read, 100) == ESP_OK) {
            i2s_channel_write(tx_handle, i2s_readraw_buff, bytes_read, &bytes_write, 100);
            printf("%zu\n", bytes_read);
        }
    }
}
