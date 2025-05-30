#include "app_http_asr.h"
#include "app_spiffs.h"
#include "hal_i2s.h"
#include "esp_log.h"
#include "json_parser.h"
#include "app_http_tongyi.h"

static const char *TAG = "BAIDU ASR";
char *access_token = CONFIG_EXAMPLE_ACCESS_TOKEN;
char *url_formate = "http://vop.baidu.com/server_api?dev_pid=1537&cuid=dPKArKm9yCGIOwPoCSjTDzmIIj4cBsEV&token=%s";
esp_http_client_handle_t asr_client;
char asr_result[1024] = {};

FILE *wav_file;
size_t wav_file_size = 0;
char *wav_raw_buffer = NULL;

esp_err_t app_http_baidu_speech_recognition_event_handler(esp_http_client_event_t *evt)
{
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        ESP_LOGI(TAG, "%.*s", evt->data_len, (char *)evt->data);
        jparse_ctx_t jctx;
        int num_elem;
        int ret = json_parse_start(&jctx, (char *)evt->data, evt->data_len);
        if (ret != OS_SUCCESS) {
            ESP_LOGI(TAG, "Parser failed\n");
            return ESP_FAIL;
        }

        if (json_obj_get_array(&jctx, "result", &num_elem) == OS_SUCCESS) {
            for (int i = 0; i < num_elem; i++) {
                json_arr_get_string(&jctx, i, asr_result, sizeof(asr_result));
                ESP_LOGI(TAG, "asr result: %s\n", asr_result);
            }
            json_obj_leave_array(&jctx);
        }

        json_parse_end(&jctx);
    } else if (evt->event_id == HTTP_EVENT_ON_FINISH) {
        if (xTongyiQuestion != NULL) {
            xQueueSend(xTongyiQuestion, asr_result, 100);
            memset(asr_result, 0, sizeof(asr_result));
        }
    }

    return ESP_OK;
}

void app_http_asr(int time)
{
    hal_i2s_record("/spiffs/record.wav", time);
    wav_file = fopen("/spiffs/record.wav", "r");
    if (wav_file == NULL) {
        ESP_LOGI(TAG, "Read audio file failed");
    }
    fseek(wav_file, 0, SEEK_END);
    wav_file_size = ftell(wav_file);
    fseek(wav_file, 0, SEEK_SET);
    ESP_LOGI(TAG, "WAV File size:%zu", wav_file_size);
    wav_raw_buffer = heap_caps_malloc(wav_file_size + 1, MALLOC_CAP_SPIRAM);
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
    char *url = heap_caps_malloc(strlen(url_formate) + strlen(access_token) + 1, MALLOC_CAP_SPIRAM);
    sprintf(url, url_formate, access_token);
    config.url = url;
    asr_client = esp_http_client_init(&config);
    esp_http_client_set_method(asr_client, HTTP_METHOD_POST);
    esp_http_client_set_header(asr_client, "Content-Type", "audio/pcm;rate=16000");
    esp_http_client_set_header(asr_client, "Accept", "application/json");
    esp_http_client_set_post_field(asr_client, wav_raw_buffer, wav_file_size);

    esp_err_t err = esp_http_client_perform(asr_client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d", esp_http_client_get_status_code(asr_client), (int)esp_http_client_get_content_length(asr_client));
    } else {
        ESP_LOGI(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(asr_client);
    free(wav_raw_buffer);
    free(url);
}
