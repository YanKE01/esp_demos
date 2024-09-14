#include "hal_i2s.h"
#include "hal_sd.h"
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_log.h"

static const char *TAG = "AUDIO";
i2s_chan_handle_t rx_handle = NULL;
record_info_t record_info = {};

esp_err_t hal_i2s_microphone_init(i2s_microphone_config_t config)
{
    esp_err_t ret_val = ESP_OK;
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(config.i2s_num, I2S_ROLE_MASTER);

    ret_val |= i2s_new_channel(&chan_cfg, NULL, &rx_handle);
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(config.sample_rate),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(config.bits_per_sample, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = GPIO_NUM_NC,
            .bclk = config.bclk_pin,
            .ws = config.ws_pin,
            .dout = GPIO_NUM_NC,
            .din = config.din_pin,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
    ret_val |= i2s_channel_init_std_mode(rx_handle, &std_cfg);
    ret_val |= i2s_channel_enable(rx_handle);
    record_info.i2s_config = config;
    return ret_val;
}

void hal_i2s_record(char *file_path, int record_time)
{
    ESP_LOGI(TAG, "Start Record");
    record_info.flash_wr_size = 0;
    record_info.byte_rate = 1 * record_info.i2s_config.sample_rate * record_info.i2s_config.bits_per_sample / 8; // 声道数×采样频率×每样本的数据位数/8。播放软件利用此值可以估计缓冲区的大小。
    record_info.bytes_all = record_info.byte_rate * record_time;                                                 // 设定时间下的所有数据大小
    record_info.sample_size = record_info.i2s_config.bits_per_sample * 1024;                                     // 每一次采样的带下
    const wav_header_t wav_header = WAV_HEADER_PCM_DEFAULT(record_info.bytes_all, record_info.i2s_config.bits_per_sample, record_info.i2s_config.sample_rate, 1);

    // 判断文件是否存在
    struct stat st;
    if (stat(file_path, &st) == 0) {
        ESP_LOGI(TAG, "%s exit", file_path);
        unlink(file_path); // 如果存在就删除
    }

    // 创建WAV文件
    FILE *f = fopen(file_path, "a");
    if (f == NULL) {
        ESP_LOGI(TAG, "Failed to open file");
        return;
    }
    fwrite(&wav_header, sizeof(wav_header), 1, f);

    while (record_info.flash_wr_size < record_info.bytes_all) {
        char *i2s_raw_buffer = heap_caps_calloc(1, record_info.sample_size, MALLOC_CAP_DMA);
        if (i2s_raw_buffer == NULL) {
            continue;
        }

        // Malloc success
        if (i2s_channel_read(rx_handle, i2s_raw_buffer, record_info.sample_size, &record_info.read_size, 1000) == ESP_OK) {
            fwrite(i2s_raw_buffer, record_info.read_size, 1, f);
            record_info.flash_wr_size += record_info.read_size;
        } else {
            ESP_LOGI(TAG, "Read Failed!\n");
        }
        free(i2s_raw_buffer);
    }

    ESP_LOGI(TAG, "Recording done!");
    fclose(f);
    ESP_LOGI(TAG, "File written on SDCard");
}
