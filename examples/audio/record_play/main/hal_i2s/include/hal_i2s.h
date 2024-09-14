#pragma once

#include "driver/i2s_common.h"
#include "driver/i2s_std.h"
#include "driver/i2s_tdm.h"
#include "driver/gpio.h"
#include "driver/i2s_pdm.h"

typedef struct {
    uint16_t sample_rate;
    uint16_t bits_per_sample;
    gpio_num_t ws_pin;
    gpio_num_t bclk_pin;
    gpio_num_t din_pin;
    i2s_port_t i2s_num;
} i2s_microphone_config_t;

typedef struct {
    uint16_t sample_rate;
    uint16_t bits_per_sample;
    gpio_num_t ws_pin;
    gpio_num_t bclk_pin;
    gpio_num_t dout_pin;
    i2s_port_t i2s_num;
} i2s_speaker_config_t;

extern i2s_chan_handle_t rx_handle;
extern i2s_chan_handle_t tx_handle;

esp_err_t hal_i2s_microphone_init(i2s_microphone_config_t config);
esp_err_t hal_i2s_speaker_init(i2s_speaker_config_t config);
