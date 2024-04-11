#pragma once

#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/i2s_pdm.h"
#include "esp_codec_dev.h"

void audio_i2s_init(i2s_port_t i2s_port, gpio_num_t i2s_dout, i2s_chan_handle_t *tx_channel, uint16_t sample_rate);
