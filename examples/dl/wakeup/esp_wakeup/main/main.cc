#include <stdio.h>
#include "predict.h"
#include "unistd.h"
#include "sys/stat.h"
#include <fcntl.h>
#include <dirent.h>
#include "hal_spiffs.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

const char *filename = "/spiffs/xiaosai_36.wav";

extern "C" void app_main(void)
{
    ESP_ERROR_CHECK(hal_spiffs_init((char *)"/spiffs"));
    wakeup_model_init();
}
