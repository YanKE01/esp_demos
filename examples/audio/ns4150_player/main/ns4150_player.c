#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "driver/i2s_pdm.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"
#include "soc/gpio_sig_map.h"
#include "soc/io_mux_reg.h"
#include "hal/rtc_io_hal.h"
#include "hal/gpio_ll.h"
#include "driver/gpio.h"
#include "audio_player.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "sdkconfig.h"

/* Audio timing */
#define AUDIO_OUTPUT_SAMPLE_RATE  24000
#define AUDIO_PDM_UPSAMPLE_FS    480

/* Pins from Kconfig (see menu "NS4150 Player Configuration") */
#define AUDIO_PDM_SPEAK_P_GPIO   ((gpio_num_t)CONFIG_NS4150_PDM_P_GPIO)
#if CONFIG_NS4150_USE_PDM_N
#define AUDIO_PDM_SPEAK_N_GPIO   ((gpio_num_t)CONFIG_NS4150_PDM_N_GPIO)
#endif
#if CONFIG_NS4150_USE_PA_CTL
#define AUDIO_PA_CTL_GPIO        ((gpio_num_t)CONFIG_NS4150_PA_CTL_GPIO)
#endif

static const char *TAG = "NS4150_PLAYER";

static const audio_codec_data_if_t *i2s_data_if = NULL;
static i2s_chan_handle_t i2s_tx_chan;
static esp_codec_dev_handle_t play_dev_handle;

/* Queue for signalling "play finished" (IDLE/SHUTDOWN) so we can play next file */
static QueueHandle_t s_play_done_queue = NULL;

static void audio_app_callback(audio_player_cb_ctx_t *ctx)
{
    switch (ctx->audio_event) {
    case 0:
        ESP_LOGI(TAG, "PLAYER IDLE");
        if (s_play_done_queue != NULL) {
            uint8_t done = 1;
            xQueueSend(s_play_done_queue, &done, 0);
        }
        break;
    case 1:
        ESP_LOGI(TAG, "PLAYER NEXT");
        break;
    case 2:
        ESP_LOGI(TAG, "PLAYER PLAYING");
        break;
    case 3:
        ESP_LOGI(TAG, "PLAYER PAUSE");
        break;
    case 4:
        ESP_LOGI(TAG, "PLAYER SHUTDOWN");
        if (s_play_done_queue != NULL) {
            uint8_t done = 1;
            xQueueSend(s_play_done_queue, &done, 0);
        }
        break;
    case 5:
        ESP_LOGI(TAG, "PLAYER UNKNOWN FILE");
        break;
    default:
        ESP_LOGI(TAG, "PLAYER UNKNOWN EVENT: %d", ctx->audio_event);
        break;
    }
}

static esp_err_t app_mute_function(AUDIO_PLAYER_MUTE_SETTING setting)
{
    return ESP_OK;
}

static esp_err_t bsp_audio_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms)
{
    esp_err_t ret = ESP_OK;
    ret = esp_codec_dev_write(play_dev_handle, audio_buffer, len);
    *bytes_written = len;
    return ret;
}

esp_err_t app_audio_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms)
{
    esp_err_t ret = ESP_OK;

    if (bsp_audio_write(audio_buffer, len, bytes_written, 1000) != ESP_OK) {
        ESP_LOGE(TAG, "Write Task: i2s write failed");
        ret = ESP_FAIL;
    }

    return ret;
}

static esp_err_t bsp_audio_reconfig_clk(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch)
{
    esp_err_t ret = ESP_OK;

    esp_codec_dev_sample_info_t fs = {
        .sample_rate = rate,
        .channel = ch,
        .bits_per_sample = bits_cfg,
    };

    ret = esp_codec_dev_close(play_dev_handle);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = esp_codec_dev_open(play_dev_handle, &fs);
    if (ret != ESP_OK) {
        return ret;
    }

    /* Reconfig PDM TX clock to match rate and up_sample_fs */
    esp_err_t err_abort = i2s_channel_disable(i2s_tx_chan);
    if (err_abort == ESP_OK) {
        i2s_pdm_tx_clk_config_t clk_cfg = I2S_PDM_TX_CLK_DEFAULT_CONFIG(rate);
        clk_cfg.up_sample_fs = AUDIO_PDM_UPSAMPLE_FS;
        err_abort = i2s_channel_reconfig_pdm_tx_clock(i2s_tx_chan, &clk_cfg);
        if (err_abort == ESP_OK) {
            err_abort = i2s_channel_enable(i2s_tx_chan);
        }
        if (err_abort != ESP_OK) {
            ESP_LOGE(TAG, "PDM reconfig failed: %s", esp_err_to_name(err_abort));
            ret = err_abort;
        }
    }
    return ret;
}

void app_main(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &i2s_tx_chan, NULL));

    i2s_pdm_tx_config_t pdm_cfg_default = {
        .clk_cfg = I2S_PDM_TX_CLK_DEFAULT_CONFIG(AUDIO_OUTPUT_SAMPLE_RATE),
        .slot_cfg = I2S_PDM_TX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .clk = GPIO_NUM_NC,
            .dout = AUDIO_PDM_SPEAK_P_GPIO,
            .invert_flags = {
                .clk_inv = false,
            },
        },
    };

    pdm_cfg_default.clk_cfg.up_sample_fs = AUDIO_PDM_UPSAMPLE_FS;
    pdm_cfg_default.slot_cfg.sd_scale = I2S_PDM_SIG_SCALING_MUL_4;
    pdm_cfg_default.slot_cfg.hp_scale = I2S_PDM_SIG_SCALING_MUL_4;
    pdm_cfg_default.slot_cfg.lp_scale = I2S_PDM_SIG_SCALING_MUL_4;
    pdm_cfg_default.slot_cfg.sinc_scale = I2S_PDM_SIG_SCALING_MUL_4;

    ESP_ERROR_CHECK(i2s_channel_init_pdm_tx_mode(i2s_tx_chan, &pdm_cfg_default));
    ESP_ERROR_CHECK(i2s_channel_enable(i2s_tx_chan));
    gpio_set_drive_capability(AUDIO_PDM_SPEAK_P_GPIO, GPIO_DRIVE_CAP_0);

    audio_codec_i2s_cfg_t i2s_cfg = {
        .port = I2S_NUM_0,
        .rx_handle = NULL,
        .tx_handle = i2s_tx_chan,
    };
    i2s_data_if = audio_codec_new_i2s_data(&i2s_cfg);

    esp_codec_dev_cfg_t codec_dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = NULL,
        .data_if = i2s_data_if,
    };

    play_dev_handle = esp_codec_dev_new(&codec_dev_cfg);
    if (play_dev_handle == NULL) {
        printf("Failed to create codec device\n");
        return;
    }

    /* Set speaker output volume to a reasonable level */
    esp_codec_dev_set_out_vol(play_dev_handle, 75);

#if CONFIG_NS4150_USE_PA_CTL
    /* PA control GPIO (high = amplifier on) */
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << AUDIO_PA_CTL_GPIO);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
    gpio_set_level(AUDIO_PA_CTL_GPIO, 1);
#endif

#if CONFIG_NS4150_USE_PDM_N
    /* PDM N: inverted SD OUT for differential output */
    PIN_FUNC_SELECT(IO_MUX_GPIO10_REG, PIN_FUNC_GPIO);
    gpio_set_direction(AUDIO_PDM_SPEAK_N_GPIO, GPIO_MODE_OUTPUT);
    esp_rom_gpio_connect_out_signal(AUDIO_PDM_SPEAK_N_GPIO, I2SO_SD_OUT_IDX, 1, 0);
    gpio_set_drive_capability(AUDIO_PDM_SPEAK_N_GPIO, GPIO_DRIVE_CAP_0);
#endif

    audio_player_config_t config = {
        .mute_fn = app_mute_function,
        .write_fn = app_audio_write,
        .clk_set_fn = bsp_audio_reconfig_clk,
        .priority = 5
    };
    ESP_ERROR_CHECK(audio_player_new(config));

    s_play_done_queue = xQueueCreate(2, sizeof(uint8_t));
    if (s_play_done_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create play-done queue");
        return;
    }
    audio_player_callback_register(audio_app_callback, NULL);

    /* SPIFFS */
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "storage",
        .max_files = 5,
        .format_if_mount_failed = false,
    };
    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));

    size_t total = 0, used = 0;
    esp_err_t ret_val = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret_val != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret_val));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    /* Play all clips in sequence (player will fclose each FILE when done) */
    static const char *play_list[] = {
        "/spiffs/start_work.wav",
        "/spiffs/need_approve.wav",
        "/spiffs/fail.wav",
        "/spiffs/success.wav",
    };
    const size_t play_count = sizeof(play_list) / sizeof(play_list[0]);

    uint8_t drain;
    while (xQueueReceive(s_play_done_queue, &drain, 0) == pdTRUE) { }

    for (size_t i = 0; i < play_count; i++) {
        FILE *fp = fopen(play_list[i], "rb");
        if (fp == NULL) {
            ESP_LOGE(TAG, "Failed to open: %s", play_list[i]);
            continue;
        }
        esp_err_t ret = audio_player_play(fp);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to play %s (%s)", play_list[i], esp_err_to_name(ret));
            fclose(fp);
            continue;
        }
        uint8_t done = 0;
        if (xQueueReceive(s_play_done_queue, &done, pdMS_TO_TICKS(30000)) != pdTRUE) {
            ESP_LOGW(TAG, "Play timeout: %s", play_list[i]);
        }
    }

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
