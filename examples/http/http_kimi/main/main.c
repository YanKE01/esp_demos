#include <stdio.h>
#include "app_wifi.h"
#include "esp_check.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

static const char *TAG = "KIMI";
char *kimi_url = "https://api.moonshot.cn/v1/chat/completions";
char *kimi_key = "sk-L1wBIZP4FYIaNlNDjssoqVyVD08MVLtuxYhwfzv10IqSWCNZ";
char *kimi_authorization_formate = "Bearer %s";
char *kimi_message = "{\"model\": \"moonshot-v1-8k\",\"messages\": [{\"role\": \"system\", \"content\": \"you are Kimi\"},{\"role\": \"user\", \"content\": \"hello,good morning\"}],\"temperature\": 0.3}";
esp_http_client_handle_t client;

esp_err_t app_http_kimi_event_handler(esp_http_client_event_t *evt)
{
    if (evt->event_id == HTTP_EVENT_ON_DATA)
    {
        ESP_LOGI(TAG, "%.*s", evt->data_len, (char *)evt->data);
    }

    return ESP_OK;
}

void app_main(void)
{
    // Init NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Connect wifi
    app_wifi_init("1401", "15201882219");

    // Set http
    esp_http_client_config_t config = {
        .method = HTTP_METHOD_POST,
        .event_handler = app_http_kimi_event_handler,
        .buffer_size = 4 * 1024,
    };
    config.url = kimi_url;
    client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    char *authorization = heap_caps_calloc(1, strlen(kimi_key) + strlen(kimi_authorization_formate), MALLOC_CAP_DMA);
    sprintf(authorization, kimi_authorization_formate, kimi_key);
    esp_http_client_set_header(client, "Authorization", authorization);
    esp_http_client_set_post_field(client, kimi_message, strlen(kimi_message));

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

    free(authorization);
}