#pragma once

#include "driver/i2s_common.h"
#include "driver/i2s_std.h"
#include "driver/i2s_tdm.h"
#include "driver/gpio.h"
#include "driver/i2s_pdm.h"
#include "wav_formate.h"

typedef struct
{
    uint16_t sample_rate;
    uint16_t bits_per_sample;
    gpio_num_t ws_pin;
    gpio_num_t bclk_pin;
    gpio_num_t din_pin;
    i2s_port_t i2s_num;
} i2s_microphone_config_t;

typedef struct
{
    i2s_microphone_config_t i2s_config; // i2s的配置信息
    int byte_rate;                      // 1s下的采样数据
    int bytes_all;                      // 录音时间下的所有数据大小
    int sample_size;                    // 每一次采样的大小
    int flash_wr_size;                  // 当前录音的大小
    size_t read_size;                   // i2s读出的长度
} record_info_t;

extern i2s_chan_handle_t rx_handle;

esp_err_t hal_i2s_microphone_init(i2s_microphone_config_t config);
void hal_i2s_record(char *file_path, int record_time);