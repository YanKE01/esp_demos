#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif
    /**
     * @brief spiffs init
     *
     * @param path mount path
     * @return esp_err_t
     */
    esp_err_t hal_spiffs_init(char *path);

#ifdef __cplusplus
}
#endif
