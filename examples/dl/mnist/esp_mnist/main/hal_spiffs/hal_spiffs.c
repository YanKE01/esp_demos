#include "hal_spiffs.h"
#include "esp_log.h"
#include "esp_spiffs.h"

static const char *TAG = "HAL_SPIFFS";

esp_err_t hal_spiffs_init(char *path)
{
    esp_err_t ret;
    esp_vfs_spiffs_conf_t conf = {
        .base_path = path,
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true, // if the mount fails, the file system will be formatted
    };

    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));

    // check spiffs
    ret = esp_spiffs_check(conf.partition_label);
    if (ret != ESP_OK) {
        ESP_LOGI(TAG, "SPIFFS Check failed:%s", esp_err_to_name(ret));
        return ESP_FAIL;
    } else {
        ESP_LOGI(TAG, "SPIFFS Check success");
    }

    // get spiffs info
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGI(TAG, "Failed to get spiffs partition info:%s", esp_err_to_name(ret));
        return ESP_FAIL;
    } else {
        ESP_LOGI(TAG, "Partition size: total:%d,used:%d ", total, used);
    }

    // if used > total，check again
    if (used > total) {
        ESP_LOGW(TAG, "Number of used bytes cannot be larger than total. Performing SPIFFS_check().");
        ret = esp_spiffs_check(conf.partition_label);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
            return ESP_FAIL;
        } else {
            ESP_LOGI(TAG, "SPIFFS_check() successful");
        }
    }
    return ESP_OK;
}
