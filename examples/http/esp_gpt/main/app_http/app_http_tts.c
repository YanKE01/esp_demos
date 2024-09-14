#include "app_http_tts.h"
#include "esp_log.h"
#include "app_url_encode.h"
#include "hal_i2s.h"

static const char *TAG = "BAIDU TTS";
esp_http_client_handle_t tts_client;
static char *url = "https://tsn.baidu.com/text2audio";
static char *access_token = "24.23d20894d86ee25c834f2f15c757d225.2592000.1715916123.282335-60592936";
static char *formate = "tex=%s&tok=%s&cuid=mpBNOBqqTHmz93GbNEZDm5vUnwV0Lnm1&ctp=1&lan=zh&spd=5&pit=5&vol=0&per=4&aue=4"; // PCM 16K

esp_err_t app_http_baidu_tts_event_handler(esp_http_client_event_t *evt)
{
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        ESP_LOGI(TAG, "Received length:%d", evt->data_len);
        //  i2s pcm
        i2s_channel_write(tx_handle, (char *)evt->data, evt->data_len, NULL, 50);
    }

    return ESP_OK;
}

esp_err_t app_http_baidu_tts_set_text(char *text)
{
    size_t text_url_encode_size = 0;
    esp_err_t err = ESP_OK;

    // 对txt进行url编码
    url_encode((unsigned char *)text, strlen(text), &text_url_encode_size, NULL, 0);
    ESP_LOGI(TAG, "text size after url encode:%zu", text_url_encode_size);
    char *text_url_encode = heap_caps_malloc(text_url_encode_size + 1, MALLOC_CAP_DMA);
    if (text_url_encode == NULL) {
        ESP_LOGI(TAG, "Malloc text url encode failed");
        return ESP_FAIL;
    }
    url_encode((unsigned char *)text, strlen(text), &text_url_encode_size, (unsigned char *)text_url_encode, text_url_encode_size + 1);

    // 设置http请求
    esp_http_client_config_t config = {
        .method = HTTP_METHOD_POST,
        .event_handler = app_http_baidu_tts_event_handler,
        .buffer_size = 10 * 1024,
        .timeout_ms = 5 * 1000,
    };
    config.url = url;
    tts_client = esp_http_client_init(&config);
    esp_http_client_set_method(tts_client, HTTP_METHOD_POST);
    esp_http_client_set_header(tts_client, "Content-Type", "application/x-www-form-urlencoded");
    esp_http_client_set_header(tts_client, "Accept", "*/*");
    char *payload = heap_caps_malloc(strlen(formate) + strlen(access_token) + strlen(text_url_encode) + 1, MALLOC_CAP_DMA);

    if (payload == NULL) {
        free(text_url_encode);
        ESP_LOGI(TAG, "Malloc payload failed");
        return ESP_FAIL;
    }

    sprintf(payload, formate, text_url_encode, access_token);
    esp_http_client_set_post_field(tts_client, payload, strlen(payload));

    err = esp_http_client_perform(tts_client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d", esp_http_client_get_status_code(tts_client), (int)esp_http_client_get_content_length(tts_client));
    } else {
        ESP_LOGI(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(tts_client);

    free(text_url_encode);
    free(payload);
    return err;
}
