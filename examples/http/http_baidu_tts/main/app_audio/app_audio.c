#include "app_audio.h"
#include "esp_codec_dev_defaults.h"
#include "esp_log.h"

void audio_i2s_init(i2s_port_t i2s_port, gpio_num_t i2s_dout, i2s_chan_handle_t *tx_channel, uint16_t sample_rate)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(i2s_port, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, tx_channel, NULL));

    i2s_pdm_tx_config_t pdm_cfg = {
        .clk_cfg = I2S_PDM_TX_CLK_DEFAULT_CONFIG(sample_rate),
        .slot_cfg = I2S_PDM_TX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg.clk = GPIO_NUM_NC,
        .gpio_cfg.dout = i2s_dout,
        .gpio_cfg.invert_flags.clk_inv = false,
    };

    // i2s通道初始化:PDM模式
    ESP_ERROR_CHECK(i2s_channel_init_pdm_tx_mode(*tx_channel, &pdm_cfg));
    // i2s通道使能
    ESP_ERROR_CHECK(i2s_channel_enable(*tx_channel));
}
