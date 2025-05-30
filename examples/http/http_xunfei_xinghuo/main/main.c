#include <stdio.h>
#include "app_wifi.h"
#include "esp_check.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

char *xf_url = "https://spark-api-open.xf-yun.com/v1/chat/completions";
char *xf_key = CONFIG_EXAMPLE_XF_KEY;
char *xf_secret = CONFIG_EXAMPLE_XF_SECRET;
char *xf_authorization_formate = "Bearer %s:%s";
char *xf_message = "{\"model\": \"generalv3.5\", \"messages\": [{\"role\": \"user\", \"content\": \"你是谁\"}]}";

esp_http_client_handle_t client;
static const char *TAG = "XF";

esp_err_t app_http_xf_event_handler(esp_http_client_event_t *evt)
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

    // Connect wifi
    app_wifi_init(CONFIG_EXAMPLE_WIFI_SSID, CONFIG_EXAMPLE_WIFI_PSWD);

    // Set http
    esp_http_client_config_t config = {
        .method = HTTP_METHOD_POST,
        .event_handler = app_http_xf_event_handler,
        .buffer_size = 4 * 1024,
        .timeout_ms = 10 * 1000,
    };
    config.url = xf_url;
    client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    char *authorization = heap_caps_calloc(1, strlen(xf_key) + strlen(xf_secret) + strlen(xf_authorization_formate), MALLOC_CAP_DMA);
    sprintf(authorization, xf_authorization_formate, xf_key, xf_secret);
    esp_http_client_set_header(client, "Authorization", authorization);
    esp_http_client_set_post_field(client, xf_message, strlen(xf_message));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d", esp_http_client_get_status_code(client), (int)esp_http_client_get_content_length(client));
    } else {
        ESP_LOGI(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);

    free(authorization);
}
