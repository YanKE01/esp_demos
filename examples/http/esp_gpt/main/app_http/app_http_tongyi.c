#include "app_http_tongyi.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "json_parser.h"
#include "app_http_tts.h"

esp_http_client_handle_t tongyi_client;
QueueHandle_t xTongyiQuestion;
static const char *TAG = "TONGYI";
char *tongyi_url = "https://dashscope.aliyuncs.com/api/v1/services/aigc/text-generation/generation";
char *key = "";
char *format = "{\"model\": \"qwen-turbo\",\"input\": {\"messages\": [{\"role\": \"system\",\"content\": \"You are a helpful assistant.\"},{\"role\": \"user\",\"content\": \"%s\"}]},\"parameters\": {\"result_format\": \"message\"}}";
char tongyi_result[1024];

esp_err_t app_http_tongyi_event_handler(esp_http_client_event_t *evt)
{
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        jparse_ctx_t jctx;
        int num_elem;
        int ret = json_parse_start(&jctx, (char *)evt->data, evt->data_len);
        if (ret != OS_SUCCESS) {
            ESP_LOGI(TAG, "Parser failed\n");
            return ESP_FAIL;
        }

        if (json_obj_get_object(&jctx, "output") == OS_SUCCESS) {
            if (json_obj_get_array(&jctx, "choices", &num_elem) == OS_SUCCESS) {
                for (int i = 0; i < num_elem; i++) {
                    if (json_arr_get_object(&jctx, i) == OS_SUCCESS) {
                        if (json_obj_get_object(&jctx, "message") == OS_SUCCESS) {
                            if (json_obj_get_string(&jctx, "content", tongyi_result, sizeof(tongyi_result)) == OS_SUCCESS) {
                                ESP_LOGI(TAG, "Tongyi:%s", tongyi_result);
                                app_http_baidu_tts_set_text(tongyi_result);
                            }
                        }
                    }
                }
            }
        }

        json_parse_end(&jctx);
    }

    return ESP_OK;
}

void app_http_ask_tongyi_task(void *pvParameters)
{
    xTongyiQuestion = xQueueCreate(10, 1024);
    char question[1024] = {0};
    while (1) {
        xQueueReceive(xTongyiQuestion, question, portMAX_DELAY);
        char *data = heap_caps_calloc(1, strlen(format) + strlen(question) + 1, MALLOC_CAP_SPIRAM);
        sprintf(data, format, question);

        esp_http_client_config_t config = {
            .method = HTTP_METHOD_POST,
            .event_handler = app_http_tongyi_event_handler,
            .buffer_size = 2 * 1024,
            .timeout_ms = 10 * 1000,
        };
        config.url = tongyi_url;
        tongyi_client = esp_http_client_init(&config);
        esp_http_client_set_method(tongyi_client, HTTP_METHOD_POST);
        esp_http_client_set_header(tongyi_client, "Content-Type", "application/json");
        esp_http_client_set_header(tongyi_client, "Authorization", key);
        esp_http_client_set_post_field(tongyi_client, data, strlen(data));
        esp_err_t err = esp_http_client_perform(tongyi_client);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d", esp_http_client_get_status_code(tongyi_client), (int)esp_http_client_get_content_length(tongyi_client));
        } else {
            ESP_LOGI(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
        }
        esp_http_client_cleanup(tongyi_client);
        free(data);
    }
}
