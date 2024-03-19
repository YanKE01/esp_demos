#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "nvs_flash.h"
#include "app_wifi.h"
#include "hal_spiffs.h"
#include "app_baidu.h"

static const char *TAG = "HTTP_BAIDU";

char *file_buf = NULL;

void app_main(void)
{
    ESP_LOGI(TAG, "Http Baidu Image Classification");
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

    // Mount spiffs
    ESP_ERROR_CHECK(hal_spiffs_init("/spiffs"));

    const char *filename = "/spiffs/taylor.jpg"; /*!< filename */
    FILE *f = fopen(filename, "rb");
    if (f == NULL)
    {
        ESP_LOGI(TAG, "Open %s failed", filename);
        return;
    }

    fseek(f, 0, SEEK_END);
    size_t filesize = ftell(f);
    rewind(f);
    ESP_LOGI(TAG, "File size:%zu", filesize);

    file_buf = (char *)malloc(filesize + 1); // 加1是为了存放字符串结束符
    if (file_buf == NULL)
    {
        fprintf(stderr, "Memory allocation failed.\n");
        fclose(f);
        return;
    }

    // 读取文件内容
    if (fread(file_buf, 1, filesize, f) != filesize)
    {
        fprintf(stderr, "Failed to read the file.\n");
        fclose(f);
        free(file_buf);
        return;
    }

    // 关闭文件
    fclose(f);

    app_baidu_classification(file_buf,filesize);
}
