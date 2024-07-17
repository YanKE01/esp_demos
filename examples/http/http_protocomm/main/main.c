#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "protocomm.h"
#include "protocomm_httpd.h"
#include "string.h"

static const char *TAG = "wifi_prov";

void wifi_init_sta(void)
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "Redmi_YK",
            .password = "123456789"},
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();
}

static esp_err_t my_command_handler(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen,
                                    uint8_t **outbuf, ssize_t *outlen, void *priv_data)
{
    ESP_LOGI(TAG, "Received data: %.*s", inlen, (char *)inbuf);

    const char *response = "Command processed successfully";
    *outlen = strlen(response);
    *outbuf = (uint8_t *)strdup(response);

    return ESP_OK;
}

void app_main(void)
{
    // 初始化NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 初始化Wi-Fi
    wifi_init_sta();

    // 创建Protocomm实例
    protocomm_t *pc = protocomm_new();
    if (pc == NULL)
    {
        ESP_LOGE(TAG, "Failed to create new protocomm instance");
        return;
    }

    // 添加HTTP传输
    protocomm_httpd_config_t config = {
        .data = {
            .config = PROTOCOMM_HTTPD_DEFAULT_CONFIG(),
        },
    };

    if (protocomm_httpd_start(pc, &config) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start HTTP transport");
        protocomm_delete(pc);
        return;
    }

    // 注册命令处理函数
    if (protocomm_add_endpoint(pc, "my_command", my_command_handler, NULL) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to add endpoint");
        protocomm_httpd_stop(pc);
        protocomm_delete(pc);
        return;
    }

    ESP_LOGI(TAG, "Protocomm service started successfully");
}
