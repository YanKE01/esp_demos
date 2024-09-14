#pragma once

#include "driver/i2s_common.h"
#include "driver/i2s_std.h"

typedef struct {
    uint16_t sample_rate;
    uint16_t bits_per_sample;
    gpio_num_t ws_pin;
    gpio_num_t bclk_pin;
    gpio_num_t din_pin;
    i2s_port_t i2s_num;
} i2s_config_t;

/**
 * @brief esp i2s init
 *
 * @param config
 * @return esp_err_t
 */
esp_err_t hal_i2s_init(i2s_config_t config);

/**
 * @brief esp i2s get data
 *
 * @param buffer
 * @param buffer_len
 * @return esp_err_t
 */
esp_err_t hal_i2s_get_data(int16_t *buffer, int buffer_len);
