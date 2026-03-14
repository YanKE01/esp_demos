#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "driver/gpio.h"
#include "driver/i2s_common.h"
#include "driver/i2s_std.h"
#include "esp_check.h"
#include "esp_dsp.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"

#if (CONFIG_MUSIC_SPECTRUM_FRAME_SAMPLES & (CONFIG_MUSIC_SPECTRUM_FRAME_SAMPLES - 1)) != 0
#error "CONFIG_MUSIC_SPECTRUM_FRAME_SAMPLES must be a power of two"
#endif

static const char *TAG = "MUSIC_SPECTRUM";

typedef struct {
    uint16_t hz_min;
    uint16_t hz_max;
} spectrum_band_range_t;

typedef struct {
    spectrum_band_range_t *band_ranges;
    int32_t *mic_raw_buffer;
    int16_t *mic_pcm_buffer;
    float *fft_input;
    float *fft_window;
    float *bands;
    float *levels;
    float *noise_floor;
    float dc_block_prev_x;
    float dc_block_prev_y;
    i2s_chan_handle_t rx_handle;
    led_strip_handle_t led_strip;
} music_spectrum_ctx_t;

static esp_err_t music_spectrum_alloc(music_spectrum_ctx_t **out_ctx)
{
    music_spectrum_ctx_t *ctx = calloc(1, sizeof(music_spectrum_ctx_t));
    ESP_RETURN_ON_FALSE(ctx != NULL, ESP_ERR_NO_MEM, TAG, "Failed to allocate context");

    ctx->band_ranges = calloc(CONFIG_MUSIC_SPECTRUM_MATRIX_WIDTH, sizeof(spectrum_band_range_t));
    ctx->mic_raw_buffer = calloc(CONFIG_MUSIC_SPECTRUM_FRAME_SAMPLES, sizeof(int32_t));
    ctx->mic_pcm_buffer = calloc(CONFIG_MUSIC_SPECTRUM_FRAME_SAMPLES, sizeof(int16_t));
    ctx->fft_input = calloc(CONFIG_MUSIC_SPECTRUM_FRAME_SAMPLES * 2, sizeof(float));
    ctx->fft_window = calloc(CONFIG_MUSIC_SPECTRUM_FRAME_SAMPLES, sizeof(float));
    ctx->bands = calloc(CONFIG_MUSIC_SPECTRUM_MATRIX_WIDTH, sizeof(float));
    ctx->levels = calloc(CONFIG_MUSIC_SPECTRUM_MATRIX_WIDTH, sizeof(float));
    ctx->noise_floor = calloc(CONFIG_MUSIC_SPECTRUM_MATRIX_WIDTH, sizeof(float));

    if (ctx->band_ranges == NULL || ctx->mic_raw_buffer == NULL || ctx->mic_pcm_buffer == NULL ||
            ctx->fft_input == NULL || ctx->fft_window == NULL || ctx->bands == NULL ||
            ctx->levels == NULL || ctx->noise_floor == NULL) {
        free(ctx->band_ranges);
        free(ctx->mic_raw_buffer);
        free(ctx->mic_pcm_buffer);
        free(ctx->fft_input);
        free(ctx->fft_window);
        free(ctx->bands);
        free(ctx->levels);
        free(ctx->noise_floor);
        free(ctx);
        return ESP_ERR_NO_MEM;
    }

    *out_ctx = ctx;
    return ESP_OK;
}

static bool matrix_xy_to_index(const music_spectrum_ctx_t *ctx, uint8_t x, uint8_t y, uint16_t *index)
{
    if (x >= CONFIG_MUSIC_SPECTRUM_MATRIX_WIDTH || y >= CONFIG_MUSIC_SPECTRUM_MATRIX_HEIGHT || index == NULL) {
        return false;
    }

    *index = x * CONFIG_MUSIC_SPECTRUM_MATRIX_HEIGHT + y;
    return true;
}

static esp_err_t led_matrix_set_pixel(const music_spectrum_ctx_t *ctx, uint8_t x, uint8_t y,
                                      uint8_t red, uint8_t green, uint8_t blue)
{
    uint16_t index = 0;
    if (!matrix_xy_to_index(ctx, x, y, &index)) {
        return ESP_ERR_INVALID_ARG;
    }

    return led_strip_set_pixel(ctx->led_strip, index, red, green, blue);
}

static void build_spectrum_band_ranges(music_spectrum_ctx_t *ctx)
{
    const size_t band_count = CONFIG_MUSIC_SPECTRUM_MATRIX_WIDTH;
    const float ratio = powf((float)CONFIG_MUSIC_SPECTRUM_MAX_HZ / (float)CONFIG_MUSIC_SPECTRUM_MIN_HZ,
                             1.0f / band_count);

    float start = (float)CONFIG_MUSIC_SPECTRUM_MIN_HZ;
    for (size_t i = 0; i < band_count; i++) {
        float end = start * ratio;
        if (i == (band_count - 1)) {
            end = (float)CONFIG_MUSIC_SPECTRUM_MAX_HZ;
        }

        ctx->band_ranges[i].hz_min = (uint16_t)lrintf(start);
        ctx->band_ranges[i].hz_max = (uint16_t)lrintf(end);
        if (ctx->band_ranges[i].hz_max <= ctx->band_ranges[i].hz_min) {
            ctx->band_ranges[i].hz_max = ctx->band_ranges[i].hz_min + 1;
        }

        ESP_LOGI(TAG, "band[%d] = %u-%u Hz", (int)i, ctx->band_ranges[i].hz_min, ctx->band_ranges[i].hz_max);
        start = end;
    }
}

static esp_err_t configure_led(music_spectrum_ctx_t *ctx)
{
    led_strip_config_t strip_config = {
        .strip_gpio_num = (gpio_num_t)CONFIG_MUSIC_SPECTRUM_LED_GPIO,
        .max_leds = CONFIG_MUSIC_SPECTRUM_MATRIX_WIDTH * CONFIG_MUSIC_SPECTRUM_MATRIX_HEIGHT,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags = {
            .invert_out = false,
        }
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = CONFIG_MUSIC_SPECTRUM_RMT_RES_HZ,
        .mem_block_symbols = CONFIG_MUSIC_SPECTRUM_RMT_MEM_WORDS,
        .flags = {
            .with_dma = true,
        }
    };

    ESP_RETURN_ON_ERROR(led_strip_new_rmt_device(&strip_config, &rmt_config, &ctx->led_strip),
                        TAG, "Failed to create LED strip");
    ESP_LOGI(TAG, "LED strip ready on GPIO %d", CONFIG_MUSIC_SPECTRUM_LED_GPIO);
    return ESP_OK;
}

static esp_err_t microphone_init(music_spectrum_ctx_t *ctx)
{
    esp_err_t ret = ESP_OK;
    i2s_chan_config_t chan_cfg =
        I2S_CHANNEL_DEFAULT_CONFIG((i2s_port_t)CONFIG_MUSIC_SPECTRUM_MIC_I2S_PORT, I2S_ROLE_MASTER);

    ret |= i2s_new_channel(&chan_cfg, NULL, &ctx->rx_handle);

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(CONFIG_MUSIC_SPECTRUM_MIC_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = GPIO_NUM_NC,
            .bclk = (gpio_num_t)CONFIG_MUSIC_SPECTRUM_MIC_BCLK_GPIO,
            .ws = (gpio_num_t)CONFIG_MUSIC_SPECTRUM_MIC_WS_GPIO,
            .dout = GPIO_NUM_NC,
            .din = (gpio_num_t)CONFIG_MUSIC_SPECTRUM_MIC_DIN_GPIO,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
    ret |= i2s_channel_init_std_mode(ctx->rx_handle, &std_cfg);
    ret |= i2s_channel_enable(ctx->rx_handle);
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to initialize microphone");

    ESP_LOGI(TAG, "Microphone ready: port=%d BCLK=%d WS=%d DIN=%d sample_rate=%lu",
             CONFIG_MUSIC_SPECTRUM_MIC_I2S_PORT,
             CONFIG_MUSIC_SPECTRUM_MIC_BCLK_GPIO,
             CONFIG_MUSIC_SPECTRUM_MIC_WS_GPIO,
             CONFIG_MUSIC_SPECTRUM_MIC_DIN_GPIO,
             (unsigned long)CONFIG_MUSIC_SPECTRUM_MIC_SAMPLE_RATE);
    return ESP_OK;
}

static esp_err_t spectrum_init(music_spectrum_ctx_t *ctx)
{
    esp_err_t ret = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to init FFT tables");
    dsps_wind_hann_f32(ctx->fft_window, CONFIG_MUSIC_SPECTRUM_FRAME_SAMPLES);
    build_spectrum_band_ranges(ctx);
    return ESP_OK;
}

static esp_err_t microphone_read_frame(music_spectrum_ctx_t *ctx, int16_t *pcm_buffer, size_t sample_count)
{
    size_t bytes_read = 0;
    ESP_RETURN_ON_FALSE(sample_count <= CONFIG_MUSIC_SPECTRUM_FRAME_SAMPLES, ESP_ERR_INVALID_ARG, TAG, "Frame too large");

    esp_err_t ret = i2s_channel_read(ctx->rx_handle, ctx->mic_raw_buffer, sample_count * sizeof(int32_t),
                                     &bytes_read, portMAX_DELAY);
    ESP_RETURN_ON_ERROR(ret, TAG, "Microphone read failed");

    size_t samples_read = bytes_read / sizeof(int32_t);
    for (size_t i = 0; i < samples_read; i++) {
        pcm_buffer[i] = (int16_t)(ctx->mic_raw_buffer[i] >> 14);
    }
    for (size_t i = samples_read; i < sample_count; i++) {
        pcm_buffer[i] = 0;
    }

    return ESP_OK;
}

static uint8_t level_to_height(const music_spectrum_ctx_t *ctx, float level, size_t band)
{
    float floor = ctx->noise_floor[band];
    if (floor < CONFIG_MUSIC_SPECTRUM_NOISE_FLOOR_MIN) {
        floor = CONFIG_MUSIC_SPECTRUM_NOISE_FLOOR_MIN;
    }

    float scaled = (level - floor) / CONFIG_MUSIC_SPECTRUM_LEVEL_RANGE;
    if (scaled < 0.0f) {
        scaled = 0.0f;
    }
    if (scaled > 1.0f) {
        scaled = 1.0f;
    }
    return (uint8_t)lrintf(scaled * (CONFIG_MUSIC_SPECTRUM_MATRIX_HEIGHT - 1));
}

static void pixel_color_for_y(const music_spectrum_ctx_t *ctx, uint8_t y,
                              uint8_t *red, uint8_t *green, uint8_t *blue)
{
    float t = 0.0f;
    if (CONFIG_MUSIC_SPECTRUM_MATRIX_HEIGHT > 1) {
        t = (float)y / (CONFIG_MUSIC_SPECTRUM_MATRIX_HEIGHT - 1);
    }
    *red = (uint8_t)(255.0f * t * ((float)CONFIG_MUSIC_SPECTRUM_LED_BRIGHTNESS_PERCENT / 100.0f));
    *green = (uint8_t)(255.0f * (1.0f - fabsf(2.0f * t - 1.0f)) *
                       ((float)CONFIG_MUSIC_SPECTRUM_LED_BRIGHTNESS_PERCENT / 100.0f));
    *blue = (uint8_t)(160.0f * (1.0f - t) * ((float)CONFIG_MUSIC_SPECTRUM_LED_BRIGHTNESS_PERCENT / 100.0f));
}

static uint16_t hz_to_bin(uint16_t hz)
{
    return (uint16_t)(((uint32_t)hz * CONFIG_MUSIC_SPECTRUM_FRAME_SAMPLES) / CONFIG_MUSIC_SPECTRUM_MIC_SAMPLE_RATE);
}

static void compute_spectrum(music_spectrum_ctx_t *ctx, const int16_t *samples, size_t sample_count, float *bands)
{
    const size_t fft_bins = CONFIG_MUSIC_SPECTRUM_FRAME_SAMPLES / 2;

    memset(ctx->fft_input, 0, sizeof(float) * CONFIG_MUSIC_SPECTRUM_FRAME_SAMPLES * 2);
    for (size_t i = 0; i < sample_count && i < CONFIG_MUSIC_SPECTRUM_FRAME_SAMPLES; i++) {
        float x = (float)samples[i];
        float y = x - ctx->dc_block_prev_x +
                  ((float)CONFIG_MUSIC_SPECTRUM_DC_BLOCK_ALPHA_PERMILLE / 1000.0f) * ctx->dc_block_prev_y;
        ctx->dc_block_prev_x = x;
        ctx->dc_block_prev_y = y;

        ctx->fft_input[i * 2] = y * ctx->fft_window[i];
        ctx->fft_input[i * 2 + 1] = 0.0f;
    }

    ESP_ERROR_CHECK(dsps_fft2r_fc32(ctx->fft_input, CONFIG_MUSIC_SPECTRUM_FRAME_SAMPLES));
    ESP_ERROR_CHECK(dsps_bit_rev_fc32(ctx->fft_input, CONFIG_MUSIC_SPECTRUM_FRAME_SAMPLES));

    for (size_t band = 0; band < CONFIG_MUSIC_SPECTRUM_MATRIX_WIDTH; band++) {
        float band_energy = 0.0f;
        uint16_t start_bin = hz_to_bin(ctx->band_ranges[band].hz_min);
        uint16_t end_bin = hz_to_bin(ctx->band_ranges[band].hz_max);
        if (start_bin == end_bin) {
            end_bin = start_bin + 1;
        }
        if (end_bin > fft_bins) {
            end_bin = fft_bins;
        }

        for (uint16_t bin = start_bin; bin < end_bin; bin++) {
            float real = ctx->fft_input[bin * 2];
            float imag = ctx->fft_input[bin * 2 + 1];
            band_energy += sqrtf(real * real + imag * imag);
        }

        uint16_t width = end_bin - start_bin;
        if (width == 0) {
            width = 1;
        }
        bands[band] = band_energy / width;
    }
}

static void calibrate_noise_floor(music_spectrum_ctx_t *ctx)
{
    float *noise_sum = calloc(CONFIG_MUSIC_SPECTRUM_MATRIX_WIDTH, sizeof(float));
    uint32_t frame_count = 0;

    ESP_RETURN_VOID_ON_FALSE(noise_sum != NULL, TAG, "Failed to allocate calibration buffer");

    if (CONFIG_MUSIC_SPECTRUM_CALIBRATION_MS <= 0) {
        for (size_t i = 0; i < CONFIG_MUSIC_SPECTRUM_MATRIX_WIDTH; i++) {
            ctx->noise_floor[i] = CONFIG_MUSIC_SPECTRUM_NOISE_FLOOR_MIN;
        }
        free(noise_sum);
        return;
    }

    int64_t start_us = esp_timer_get_time();
    ESP_LOGI(TAG, "Calibrating ambient noise for %lu ms", (unsigned long)CONFIG_MUSIC_SPECTRUM_CALIBRATION_MS);

    while ((esp_timer_get_time() - start_us) < ((int64_t)CONFIG_MUSIC_SPECTRUM_CALIBRATION_MS * 1000LL)) {
        ESP_ERROR_CHECK(microphone_read_frame(ctx, ctx->mic_pcm_buffer, CONFIG_MUSIC_SPECTRUM_FRAME_SAMPLES));
        compute_spectrum(ctx, ctx->mic_pcm_buffer, CONFIG_MUSIC_SPECTRUM_FRAME_SAMPLES, ctx->bands);

        for (size_t i = 0; i < CONFIG_MUSIC_SPECTRUM_MATRIX_WIDTH; i++) {
            noise_sum[i] += ctx->bands[i];
        }
        frame_count++;
    }

    if (frame_count == 0) {
        frame_count = 1;
    }

    for (size_t i = 0; i < CONFIG_MUSIC_SPECTRUM_MATRIX_WIDTH; i++) {
        ctx->noise_floor[i] = noise_sum[i] / frame_count;
        if (ctx->noise_floor[i] < CONFIG_MUSIC_SPECTRUM_NOISE_FLOOR_MIN) {
            ctx->noise_floor[i] = CONFIG_MUSIC_SPECTRUM_NOISE_FLOOR_MIN;
        }
        ESP_LOGI(TAG, "band[%d] floor=%.1f", (int)i, ctx->noise_floor[i]);
    }

    free(noise_sum);
}

static void draw_spectrum(music_spectrum_ctx_t *ctx, const float *bands)
{
    ESP_ERROR_CHECK(led_strip_clear(ctx->led_strip));

    for (uint8_t x = 0; x < CONFIG_MUSIC_SPECTRUM_MATRIX_WIDTH; x++) {
        float new_level = bands[x];
        float old_level = ctx->levels[x];
        float delta = new_level - old_level;

        if (fabsf(delta) < CONFIG_MUSIC_SPECTRUM_MIN_DELTA) {
            new_level = old_level;
        }

        float alpha = (new_level > old_level) ?
                      ((float)CONFIG_MUSIC_SPECTRUM_SMOOTH_RISE_PERCENT / 100.0f) :
                      ((float)CONFIG_MUSIC_SPECTRUM_SMOOTH_FALL_PERCENT / 100.0f);
        ctx->levels[x] = old_level + alpha * (new_level - old_level);

        uint8_t height = level_to_height(ctx, ctx->levels[x], x);
        for (uint8_t y = 0; y <= height; y++) {
            uint8_t red = 0;
            uint8_t green = 0;
            uint8_t blue = 0;
            pixel_color_for_y(ctx, y, &red, &green, &blue);
            ESP_ERROR_CHECK(led_matrix_set_pixel(ctx, x, y, red, green, blue));
        }
    }

    ESP_ERROR_CHECK(led_strip_refresh(ctx->led_strip));
}

void app_main(void)
{
    music_spectrum_ctx_t *ctx = NULL;
    ESP_ERROR_CHECK(music_spectrum_alloc(&ctx));
    ESP_ERROR_CHECK(configure_led(ctx));
    ESP_ERROR_CHECK(microphone_init(ctx));
    ESP_ERROR_CHECK(spectrum_init(ctx));

    ESP_ERROR_CHECK(led_strip_clear(ctx->led_strip));
    ESP_ERROR_CHECK(led_strip_refresh(ctx->led_strip));
    calibrate_noise_floor(ctx);

    while (1) {
        ESP_ERROR_CHECK(microphone_read_frame(ctx, ctx->mic_pcm_buffer, CONFIG_MUSIC_SPECTRUM_FRAME_SAMPLES));
        compute_spectrum(ctx, ctx->mic_pcm_buffer, CONFIG_MUSIC_SPECTRUM_FRAME_SAMPLES, ctx->bands);
        draw_spectrum(ctx, ctx->bands);
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
