#include "hal_i2s.h"
#include "esp_codec_dev_defaults.h"
#include "audio_player.h"
#include "esp_log.h"

static const char *TAG = "ADUIO";

const audio_codec_data_if_t *i2s_data_if;
i2s_chan_handle_t i2s_tx_chan;
esp_codec_dev_handle_t play_dev_handle;

const audio_codec_data_if_t *hal_i2s_init(i2s_port_t i2s_port, hal_i2s_pin_t pin_config, i2s_chan_handle_t *tx_channel, uint16_t sample_rate)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(i2s_port, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, tx_channel, NULL));

    i2s_tdm_config_t tdm_config = {
        .clk_cfg = I2S_TDM_CLK_DEFAULT_CONFIG(sample_rate),
        .slot_cfg = I2S_TDM_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO, I2S_TDM_SLOT0 | I2S_TDM_SLOT1 | I2S_TDM_SLOT2 | I2S_TDM_SLOT3),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = pin_config.bclk_pin,
            .dout = pin_config.dout_pin,
            .ws = pin_config.ws_pin,
            .invert_flags = {
                .bclk_inv = false,
                .mclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    tdm_config.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_512;
    ESP_ERROR_CHECK(i2s_channel_init_tdm_mode(*tx_channel, &tdm_config));
    ESP_ERROR_CHECK(i2s_channel_enable((*tx_channel)));

    audio_codec_i2s_cfg_t i2s_cfg = {
        .port = i2s_port,
        .rx_handle = NULL,
        .tx_handle = *tx_channel,
    };
    return audio_codec_new_i2s_data(&i2s_cfg);
}

esp_codec_dev_handle_t audio_codec_init(const audio_codec_data_if_t *data_if)
{
    assert(data_if);

    esp_codec_dev_cfg_t codec_dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = NULL,
        .data_if = data_if,
    };

    return esp_codec_dev_new(&codec_dev_cfg);
}

esp_err_t aduio_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms)
{
    esp_err_t ret = ESP_OK;
    ret = esp_codec_dev_write(play_dev_handle, audio_buffer, len);
    *bytes_written = len;
    return ret;
}

static esp_err_t aduio_app_mute_function(AUDIO_PLAYER_MUTE_SETTING setting)
{
    return ESP_OK;
}

esp_err_t aduio_app_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms)
{
    esp_err_t ret = ESP_OK;

    if (aduio_write(audio_buffer, len, bytes_written, 1000) != ESP_OK) {
        ret = ESP_FAIL;
    }

    return ret;
}

esp_err_t audio_app_reconfig_clk(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch)
{
    esp_err_t ret = ESP_OK;

    esp_codec_dev_sample_info_t fs = {
        .sample_rate = rate,
        .channel = ch,
        .bits_per_sample = bits_cfg,
    };
    ret = esp_codec_dev_close(play_dev_handle);
    ret = esp_codec_dev_open(play_dev_handle, &fs);
    return ret;
}

esp_err_t audio_app_player_music(char *file_path)
{
    FILE *fp = fopen(file_path, "r");
    return audio_player_play(fp);
}

static void audio_app_callback(audio_player_cb_ctx_t *ctx)
{
    switch (ctx->audio_event) {
    case 0: /**< Player is idle, not playing audio */
        ESP_LOGI(TAG, "IDLE");
        break;
    case 1:
        ESP_LOGI(TAG, "NEXT");
        break;
    case 2:
        ESP_LOGI(TAG, "PLAYING");
        break;
    case 3:
        ESP_LOGI(TAG, "PAUSE");
        break;
    case 4:
        ESP_LOGI(TAG, "SHUTDOWN");
        break;
    case 5:
        ESP_LOGI(TAG, "UNKNOWN FILE");
        break;
    case 6:
        ESP_LOGI(TAG, "UNKNOWN");
        break;
    }
}

esp_err_t audio_app_player_init(i2s_port_t i2s_port, hal_i2s_pin_t pin_cfg, uint16_t sample_rate)
{
    i2s_data_if = hal_i2s_init(i2s_port, pin_cfg, &i2s_tx_chan, sample_rate);
    if (i2s_data_if == NULL) {
        return ESP_FAIL;
    }

    play_dev_handle = audio_codec_init(i2s_data_if);
    if (play_dev_handle == NULL) {
        return ESP_FAIL;
    }

    esp_codec_dev_set_out_vol(play_dev_handle, 50); // Set volume

    audio_player_config_t config = {
        .mute_fn = aduio_app_mute_function,
        .write_fn = aduio_app_write,
        .clk_set_fn = audio_app_reconfig_clk,
        .priority = 5,
    };
    ESP_ERROR_CHECK(audio_player_new(config));
    audio_player_callback_register(audio_app_callback, NULL);
    return ESP_OK;
}
