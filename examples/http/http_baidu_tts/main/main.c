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

static const char *TAG = "HTTP_BAIDU";
char *url = "https://tsn.baidu.com/text2audio";

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
    app_wifi_init("MERCURY_5B00", "tzyjy12345678");

    // Http
}
