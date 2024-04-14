#include "hal_i2s.h"

i2s_chan_handle_t rx_handle = NULL;
i2s_chan_handle_t tx_handle = NULL;

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
    return ret_val;
}

esp_err_t hal_i2s_speaker_init(i2s_speaker_config_t config)
{
    esp_err_t ret_val = ESP_OK;

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(config.i2s_num, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle, NULL));

    i2s_tdm_config_t tdm_config = {
        .clk_cfg = I2S_TDM_CLK_DEFAULT_CONFIG(config.sample_rate),
        .slot_cfg = I2S_TDM_MSB_SLOT_DEFAULT_CONFIG(config.bits_per_sample, I2S_SLOT_MODE_MONO, I2S_TDM_SLOT0),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = config.bclk_pin,
            .dout = config.dout_pin,
            .ws = config.ws_pin,
            .invert_flags = {
                .bclk_inv = false,
                .mclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    tdm_config.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_512;
    ret_val |= i2s_channel_init_tdm_mode(tx_handle, &tdm_config);
    ret_val |= i2s_channel_enable((tx_handle));
    return ret_val;
}