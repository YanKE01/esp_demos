#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "predict.h"
#include "unistd.h"
#include "sys/stat.h"
#include <fcntl.h>
#include <dirent.h>
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "hal_i2s.h"
#include "c_speech_features.h"
#include "string.h"
#include "math.h"
#include "led_strip.h"

static const char *TAG = "wakeup";
led_strip_handle_t led_strip = NULL;

i2s_config_t i2s_config = {
    .sample_rate = 16000,
    .bits_per_sample = 32,
    .ws_pin = GPIO_NUM_42,
    .bclk_pin = GPIO_NUM_41,
    .din_pin = GPIO_NUM_2,
    .i2s_num = I2S_NUM_1,
};

void esp_wakupnet_task(void *args)
{
    int total_audio_length = 23552;
    int chunk_size = 5888;
    float max_val = 0.0f;
    int win_length = (int)(0.02 * 16000);
    float window[win_length];
    int num_cep = 13;

    for (int i = 0; i < win_length; i++)
    {
        window[i] = 1.0f;
    }

    while (1)
    {
        float *result = NULL;
        int16_t *audio_data = (int16_t *)heap_caps_calloc(1, total_audio_length * sizeof(int16_t), MALLOC_CAP_SPIRAM);
        if (audio_data == NULL)
        {
            ESP_LOGE(TAG, "Memory allocation failed");
            break;
        }

        ESP_ERROR_CHECK(hal_i2s_get_data_chunks(audio_data, total_audio_length, chunk_size));

        // find max
        for (int i = 0; i < total_audio_length; i++)
        {
            float abs_val = fabs((float)audio_data[i]);
            if (abs_val > max_val)
            {
                max_val = abs_val;
            }
        }
        short *normalized_signal = (short *)heap_caps_calloc(1, total_audio_length * sizeof(short), MALLOC_CAP_SPIRAM);
        if (normalized_signal == NULL)
        {
            ESP_LOGE(TAG, "Memory allocation failed");
            break;
        }

        for (int i = 0; i < total_audio_length; i++)
        {
            normalized_signal[i] = (short)(roundf((float)audio_data[i] / max_val * 32767));
        }

        int n_frames = csf_mfcc(normalized_signal, total_audio_length, 16 * 1000, 0.02, 0.02, 13, 32, 512, 0, 16000 / 2,
                                0.98, 32, 1, window, &result);

        wakeup_model_predict(result, num_cep * n_frames);
        free(audio_data);
        free(normalized_signal);
        free(result);
    }

    vTaskDelete(NULL);
}

extern "C" void app_main(void)
{
    wakeup_model_init();
    ESP_ERROR_CHECK(hal_i2s_init(i2s_config));

    led_strip_config_t strip_config = {
        .strip_gpio_num = GPIO_NUM_38,            // The GPIO that connected to the LED strip's data line
        .max_leds = 1,                            // The number of LEDs in the strip,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB, // Pixel format of your LED strip
        .led_model = LED_MODEL_WS2812,            // LED strip model
    };
    strip_config.flags.invert_out = false;

    led_strip_rmt_config_t rmt_config = {
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
        .rmt_channel = 0,
#else
        .clk_src = RMT_CLK_SRC_DEFAULT,    // different clock source can lead to different power consumption
        .resolution_hz = 10 * 1000 * 1000, // RMT counter clock frequency
#endif
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    led_strip_clear(led_strip);

    xTaskCreatePinnedToCore(esp_wakupnet_task, "wakeup", 10 * 1024, NULL, 5, NULL, 0);
}
