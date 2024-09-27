#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/rmt_tx.h"

void app_main(void)
{
    rmt_tx_channel_config_t tx_channel_cfg = {
        .mem_block_symbols = SOC_RMT_MEM_WORDS_PER_CHANNEL,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 1 * 1000 * 1000, // 1MHz, 1 tick = 1us
        .trans_queue_depth = 4,
        .gpio_num = GPIO_NUM_4,
        .intr_priority = 3
    };
    rmt_channel_handle_t tx_channel = NULL;
    rmt_new_tx_channel(&tx_channel_cfg, &tx_channel);

    rmt_encoder_handle_t bytes_encoder = NULL;

    // Encode bit 0 and 1
    rmt_bytes_encoder_config_t bytes_enc_config = {
        .bit0 = {
            .level0 = 1,
            .duration0 = 1, // 1us
            .level1 = 0,
            .duration1 = 1, // 1us
        }, // For bit 0: first high level for 1us, then low level for 1us
        .bit1 = {
            .level0 = 1,
            .duration0 = 9, // 9us
            .level1 = 0,
            .duration1 = 3, // 3us
        }, // For bit 1: first high level for 9us, then low level for 3us
    };

    rmt_new_bytes_encoder(&bytes_enc_config, &bytes_encoder);
    rmt_enable(tx_channel);

    rmt_transmit_config_t transmit_config = {
        .loop_count = 0, // no loop
    };

    while (1) {
        rmt_transmit(tx_channel, bytes_encoder, (uint8_t[]) {
            0x05
        }, 1, &transmit_config); // For 0x05, it's actually 8-bit data, corresponding to 0000 0101
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}
