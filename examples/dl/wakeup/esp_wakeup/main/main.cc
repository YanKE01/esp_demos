#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "predict.h"
#include "unistd.h"
#include "sys/stat.h"
#include <fcntl.h>
#include <dirent.h>
#include "hal_spiffs.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "format_wav.h"

const char *filename = "/spiffs/xiaosai.wav";
static const char *TAG = "wakeup";

extern "C" void app_main(void)
{
    ESP_ERROR_CHECK(hal_spiffs_init((char *)"/spiffs"));
    wakeup_model_init();

    FILE *wav_file = fopen(filename, "r");
    if (wav_file == NULL)
    {
        ESP_LOGI(TAG, "Read audio file failed");
    }

    fseek(wav_file, 0, SEEK_END);
    size_t wav_file_size = ftell(wav_file);
    fseek(wav_file, 0, SEEK_SET);
    ESP_LOGI(TAG, "WAV File size:%zu", wav_file_size);

    char *wav_raw_buffer = (char *)heap_caps_malloc(wav_file_size + 1, MALLOC_CAP_SPIRAM);
    if (wav_raw_buffer == NULL)
    {
        ESP_LOGI(TAG, "Malloc wav raw buffer fail");
        return;
    }
    fread(wav_raw_buffer, 1, wav_file_size, wav_file);
    fclose(wav_file);
    free(wav_raw_buffer);
    ESP_LOGI(TAG, "Finish");
}
