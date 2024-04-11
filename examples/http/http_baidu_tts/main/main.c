#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "nvs_flash.h"
#include "string.h"
#include "app_wifi.h"
#include "app_url_encode.h"
#include "esp_http_client.h"
#include "app_audio.h"

static const char *TAG = "HTTP_BAIDU";
esp_http_client_handle_t client;
char *url = "https://tsn.baidu.com/text2audio";
char *access_token = "24.3ff7eb10fd7de8342cf12fde2fd35194.2592000.1715402191.282335-60592936";
char *formate = "tex=%s&tok=%s&cuid=mpBNOBqqTHmz93GbNEZDm5vUnwV0Lnm1&ctp=1&lan=zh&spd=5&pit=5&vol=5&per=4&aue=4"; // PCM 16K
char *text = "早上好,下午好,晚上好";
size_t text_url_encode_size = 0;

i2s_chan_handle_t i2s_tx_chan;

esp_err_t app_http_baidu_tts_event_handler(esp_http_client_event_t *evt)
{
    if (evt->event_id == HTTP_EVENT_ON_DATA)
    {
        ESP_LOGI(TAG, "Received length:%d", evt->data_len);
        i2s_channel_write(i2s_tx_chan, (char *)evt->data, evt->data_len, NULL, 100);
    }

    return ESP_OK;
}

void app_main(void)
{
    ESP_LOGI(TAG, "Http Baidu TTS");

    // Init NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Connect WIFI
    // app_wifi_init("ChinaUnicom-3LRNAS", "244244244");
    app_wifi_init("MERCURY_5B00", "tzyjy12345678");

    // Init audio
    audio_i2s_init(I2S_NUM_0, GPIO_NUM_3, &i2s_tx_chan, 16 * 1000);

    // Http
    esp_http_client_config_t config = {
        .method = HTTP_METHOD_POST,
        .event_handler = app_http_baidu_tts_event_handler,
        .buffer_size = 10 * 1024,
    };

    config.url = url;
    client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");
    esp_http_client_set_header(client, "Accept", "*/*");

    url_encode((unsigned char *)text, strlen(text), &text_url_encode_size, NULL, 0);

    ESP_LOGI(TAG, "text size after url:%zu", text_url_encode_size);

    char *text_url_encode = heap_caps_calloc(1, text_url_encode_size + 1, MALLOC_CAP_DMA);

    if (text_url_encode == NULL)
    {
        ESP_LOGI(TAG, "Malloc text url encode failed");
        return;
    }

    url_encode((unsigned char *)text, strlen(text), &text_url_encode_size, (unsigned char *)text_url_encode, text_url_encode_size + 1);

    char *payload = heap_caps_calloc(1, strlen(formate) + strlen(access_token) + strlen(text_url_encode) + 1, MALLOC_CAP_DMA);

    if (payload == NULL)
    {
        free(text_url_encode);
        ESP_LOGI(TAG, "Malloc payload failed");
        return;
    }

    sprintf(payload, formate, text_url_encode, access_token);
    esp_http_client_set_post_field(client, payload, strlen(payload));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d", esp_http_client_get_status_code(client), (int)esp_http_client_get_content_length(client));
    }
    else
    {
        ESP_LOGI(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);

    free(text_url_encode);
    free(payload);
}
