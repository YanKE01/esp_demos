#include <stdio.h>
#include "esp_log.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "app_wifi.h"
#include "esp_http_client.h"
#include "console_simple_init.h"

static const char *TAG = "HTTP_TONGYI";
esp_http_client_handle_t client;
QueueHandle_t xQueue;
char *tongyi_url = "https://dashscope.aliyuncs.com/api/v1/services/aigc/text-generation/generation";
char *key = "";
char *format = "{\"model\": \"qwen-turbo\",\"input\": {\"messages\": [{\"role\": \"system\",\"content\": \"You are a helpful assistant.\"},{\"role\": \"user\",\"content\": \"%s\"}]},\"parameters\": {\"result_format\": \"message\"}}";

esp_err_t app_http_tongyi_event_handler(esp_http_client_event_t *evt)
{
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        ESP_LOGI(TAG, "%.*s", evt->data_len, (char *)evt->data);
    }

    return ESP_OK;
}

void app_ask_tongyi_task(void *pvParameters)
{
    char question[1024] = {0};
    while (1) {
        xQueueReceive(xQueue, question, portMAX_DELAY);
        char *data = heap_caps_calloc(1, strlen(format) + strlen(question) + 1, MALLOC_CAP_DMA);
        sprintf(data, format, question);
        ESP_LOGI(TAG, "%s %d", question, strlen(question));

        esp_http_client_config_t config = {
            .method = HTTP_METHOD_POST,
            .event_handler = app_http_tongyi_event_handler,
            .buffer_size = 4 * 1024,
        };
        config.url = tongyi_url;
        client = esp_http_client_init(&config);
        esp_http_client_set_method(client, HTTP_METHOD_POST);
        esp_http_client_set_header(client, "Content-Type", "application/json");
        esp_http_client_set_header(client, "Authorization", key);
        esp_http_client_set_post_field(client, data, strlen(data));
        esp_err_t err = esp_http_client_perform(client);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d", esp_http_client_get_status_code(client), (int)esp_http_client_get_content_length(client));
        } else {
            ESP_LOGI(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
        }
        esp_http_client_cleanup(client);
        free(data);
    }
}

int ask_tongyi_cmd(int argc, char **argv)
{
    if (argc == 2) {
        xQueueSend(xQueue, argv[1], 100);
    } else {
        ESP_LOGI(TAG, "invalid parameters, usage: ask xxxx");
    }
    return 0;
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

    // Init queue
    xQueue = xQueueCreate(10, 1024);

    // Init console
    ESP_ERROR_CHECK(console_cmd_init());
    ESP_ERROR_CHECK(console_cmd_user_register("ask", ask_tongyi_cmd));
    ESP_ERROR_CHECK(console_cmd_all_register());
    ESP_ERROR_CHECK(console_cmd_start());

    // Init task
    xTaskCreate(app_ask_tongyi_task, "ask tongyi", 4 * 2048, NULL, 5, NULL);
}
