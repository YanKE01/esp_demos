#include <stdio.h>
#include "esp_log.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "app_wifi.h"
#include "esp_http_client.h"
#include <json_parser.h>

static const char *TAG = "BAIDU_ACCESS_TOKEN";
char *client_id = "VHmYbhR424X5zfTRooHYD7ve";
char *client_secret = "ODH92wNoceW8kXtgxoxCyHidhWfeX8YV";
char *url_format = "https://aip.baidubce.com/oauth/2.0/token?client_id=%s&client_secret=%s&grant_type=client_credentials";
char access_token[256] = {0};
esp_http_client_handle_t client;

esp_err_t app_http_baidu_access_token_event_handler(esp_http_client_event_t *evt)
{
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        // ESP_LOGI(TAG, "%.*s", evt->data_len, (char *)evt->data);
        jparse_ctx_t jctx;
        int ret = json_parse_start(&jctx, (char *)evt->data, evt->data_len);
        if (ret != OS_SUCCESS) {
            ESP_LOGI(TAG, "Parser failed\n");
            return ESP_FAIL;
        }

        if (json_obj_get_string(&jctx, "access_token", access_token, sizeof(access_token)) == OS_SUCCESS) {
            ESP_LOGI(TAG, "access_token: %s\n", access_token);
        }

        json_parse_end(&jctx);
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

    // Http
    esp_http_client_config_t config = {
        .method = HTTP_METHOD_POST,
        .event_handler = app_http_baidu_access_token_event_handler,
        .buffer_size = 4 * 1024,
    };

    char *url = heap_caps_calloc(1, strlen(url_format) + strlen(client_id) + strlen(client_secret), MALLOC_CAP_DMA);
    sprintf(url, url_format, client_id, client_secret);

    config.url = url;
    client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Accept", "application/json");
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d", esp_http_client_get_status_code(client), (int)esp_http_client_get_content_length(client));
    } else {
        ESP_LOGI(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
    free(url);
}
