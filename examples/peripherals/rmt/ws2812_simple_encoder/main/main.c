#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/rmt_tx.h"
#include "esp_log.h"

static const char *TAG = "WS2812";

static const rmt_symbol_word_t ws2812_zero = {
    .level0 = 1,
    .duration0 = 4, /*!< 0.4us */
    .level1 = 0,
    .duration1 = 9, /*!< 0.9us */
};

static const rmt_symbol_word_t ws2812_one = {
    .level0 = 1,
    .duration0 = 9, /*!< 0.9us */
    .level1 = 0,
    .duration1 = 3, /*!< 0.4us */
};

static const rmt_symbol_word_t ws2812_reset = {
    .level0 = 0,
    .duration0 = 250, /*!< 25us */
    .level1 = 0,
    .duration1 = 250, /*!< 25us */
};

static size_t encoder_callback(const void *data, size_t data_size,
                               size_t symbols_written, size_t symbols_free,
                               rmt_symbol_word_t *symbols, bool *done, void *arg)
{
    if (symbols_free < 8) {
        return 0; /*!< Need at least 8 symbols to form one byte */
    }
    printf("symbols_written:%d,data_size:%d\n", symbols_written, data_size);

    size_t data_pos = symbols_written / 8; /*!< Calculate current byte position, assuming input is {0xFF, 0x00, 0x00}, data_size is 3, each byte written increments symbols_written by 8 */
    uint8_t *data_bytes = (uint8_t *)data;

    if (data_pos < data_size) {
        size_t symbol_pos = 0;
        for (int bitmask = 0x80; bitmask != 0; bitmask >>= 1) {
            if (data_bytes[data_pos] & bitmask) {
                symbols[symbol_pos++] = ws2812_one;
            } else {
                symbols[symbol_pos++] = ws2812_zero;
            }
        }

        return symbol_pos;
    } else {
        symbols[0] = ws2812_reset;
        *done = 1;
        return 1;
    }
}

void app_main(void)
{
    rmt_channel_handle_t led_chan = NULL;
    rmt_tx_channel_config_t tx_chan_config = {
        .mem_block_symbols = 64,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000, // 10MHz, 1 tick = 0.1us
        .trans_queue_depth = 4,
        .gpio_num = GPIO_NUM_38,
        .intr_priority = 3
    };

    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &led_chan));

    ESP_LOGI(TAG, "Create simple callback-based encoder");
    rmt_encoder_handle_t simple_encoder = NULL;
    const rmt_simple_encoder_config_t simple_encoder_cfg = {
        .callback = encoder_callback
        // Note we don't set min_chunk_size here as the default of 64 is good enough.
    };
    ESP_ERROR_CHECK(rmt_new_simple_encoder(&simple_encoder_cfg, &simple_encoder));

    ESP_LOGI(TAG, "Enable RMT TX channel");
    ESP_ERROR_CHECK(rmt_enable(led_chan));

    ESP_LOGI(TAG, "Start LED rainbow chase");
    rmt_transmit_config_t tx_config = {
        .loop_count = 0, // no transfer loop
    };

    // WS2812 encoding is GRB
    uint8_t red[3] = {0x00, 0xFF, 0x00}; /*!< R */
    rmt_transmit(led_chan, simple_encoder, red, sizeof(red), &tx_config);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    uint8_t green[3] = {0xFF, 0x00, 0x00}; /*!< G */
    rmt_transmit(led_chan, simple_encoder, green, sizeof(green), &tx_config);
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    uint8_t blue[3] = {0x00, 0x00, 0xFF}; /*!< B */
    rmt_transmit(led_chan, simple_encoder, blue, sizeof(blue), &tx_config);
}
