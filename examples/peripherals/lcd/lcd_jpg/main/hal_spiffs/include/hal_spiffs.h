#pragma once

#include "esp_err.h"

/**
 * @brief spiffs init
 *
 * @param path mount path
 * @return esp_err_t
 */
esp_err_t hal_spiffs_init(char *path);
