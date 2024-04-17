#pragma once

#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"


esp_err_t app_spiffs_init(char *mount_path);
