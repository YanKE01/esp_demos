#include "hal_i2s.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "string.h"

static const char *TAG = "i2s";

i2s_chan_handle_t rx_handle = NULL; // I2S rx channel handler

esp_err_t hal_i2s_init(i2s_config_t config)
{
    esp_err_t ret_val = ESP_OK;
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(config.i2s_num, I2S_ROLE_MASTER);

    ret_val |= i2s_new_channel(&chan_cfg, NULL, &rx_handle);
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(config.sample_rate),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(config.bits_per_sample, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = GPIO_NUM_NC,
            .bclk = config.bclk_pin,
            .ws = config.ws_pin,
            .dout = GPIO_NUM_NC,
            .din = config.din_pin,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
    ret_val |= i2s_channel_init_std_mode(rx_handle, &std_cfg);
    ret_val |= i2s_channel_enable(rx_handle);
    return ret_val;
}

esp_err_t hal_i2s_get_data_chunks(int16_t *buffer, int buffer_len, int chunk_size)
{
    esp_err_t ret = ESP_OK;
    size_t bytes_read = 0;

    for (int i = 0; i < buffer_len / chunk_size; i++)
    {
        int32_t *temp = heap_caps_calloc(1, chunk_size * sizeof(int32_t), MALLOC_CAP_SPIRAM);

        if (temp == NULL)
        {
            ESP_LOGE(TAG, "Malloc temp failed");
            return ESP_FAIL;
        }

        ret = i2s_channel_read(rx_handle, temp, chunk_size * sizeof(int16_t), &bytes_read, portMAX_DELAY);

        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Read failed with error: %d", ret);
            free(temp);
            return ret;
        }

        for (int j = 0; j < chunk_size; j++)
        {
            buffer[i * chunk_size + j] = temp[j] >> 14;
        }

        free(temp);
    }

    return ret;
}