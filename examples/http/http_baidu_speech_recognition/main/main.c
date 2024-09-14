#include <stdio.h>
#include "esp_log.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "app_spiffs.h"
#include "nvs_flash.h"
#include "hal_i2s.h"
#include "app_wifi.h"
#include "esp_http_client.h"
#include "esp_heap_caps.h"

static const char *TAG = "HTTP BAIDU";

i2s_microphone_config_t i2s_microphone_config = {
    .bclk_pin = GPIO_NUM_41,
    .ws_pin = GPIO_NUM_42,
    .din_pin = GPIO_NUM_2,
    .i2s_num = I2S_NUM_1,
    .sample_rate = 16 * 1000,
    .bits_per_sample = I2S_DATA_BIT_WIDTH_16BIT,
};

char *access_token = "24.4c1cacd26dc1b0f768a3b48ca393bd8c.2592000.1715692765.282335-60592936";
char *url_formate = "http://vop.baidu.com/server_api?dev_pid=1537&cuid=dPKArKm9yCGIOwPoCSjTDzmIIj4cBsEV&token=%s";
esp_http_client_handle_t client;

FILE *wav_file;
size_t wav_file_size = 0;
char *wav_raw_buffer = NULL;

esp_err_t app_http_baidu_speech_recognition_event_handler(esp_http_client_event_t *evt)
{
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        ESP_LOGI(TAG, "%.*s", evt->data_len, (char *)evt->data);
    }

    return ESP_OK;
}

void app_main(void)
{
    // Init NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Connect WIFI
    app_wifi_init("MERCURY_5B00", "tzyjy12345678");

    // Init spiffs
    ESP_ERROR_CHECK(app_spiffs_init("/spiffs"));

    // Init i2s microphone
    ESP_ERROR_CHECK(hal_i2s_microphone_init(i2s_microphone_config));
    hal_i2s_record("/spiffs/record.wav", 5);
    wav_file = fopen("/spiffs/record.wav", "r");
    if (wav_file == NULL) {
        ESP_LOGI(TAG, "Read audio file failed");
    }
    fseek(wav_file, 0, SEEK_END);
    wav_file_size = ftell(wav_file);
    fseek(wav_file, 0, SEEK_SET);
    ESP_LOGI(TAG, "WAV File size:%zu", wav_file_size);
    wav_raw_buffer = heap_caps_malloc(wav_file_size + 1, MALLOC_CAP_DMA);
    if (wav_raw_buffer == NULL) {
        ESP_LOGI(TAG, "Malloc wav raw buffer fail");
        return;
    }
    fread(wav_raw_buffer, 1, wav_file_size, wav_file);
    fclose(wav_file);

    // HTTP
    esp_http_client_config_t config = {
        .method = HTTP_METHOD_POST,
        .event_handler = app_http_baidu_speech_recognition_event_handler,
        .buffer_size = 4 * 1024,
    };
    char *url = heap_caps_malloc(strlen(url_formate) + strlen(access_token) + 1, MALLOC_CAP_DMA);
    sprintf(url, url_formate, access_token);
    config.url = url;
    client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "audio/pcm;rate=16000");
    esp_http_client_set_header(client, "Accept", "application/json");
    esp_http_client_set_post_field(client, wav_raw_buffer, wav_file_size);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d", esp_http_client_get_status_code(client), (int)esp_http_client_get_content_length(client));
    } else {
        ESP_LOGI(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);

    free(url);
}
