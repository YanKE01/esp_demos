#include <stdio.h>
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

static const char *TAG = "spi_qio";

void app_main(void)
{
    ESP_LOGI(TAG, "QSPI TX Example");

    spi_bus_config_t buscfg = {
        .sclk_io_num = GPIO_NUM_5,
        .mosi_io_num = GPIO_NUM_1,
        .miso_io_num = GPIO_NUM_2,
        .quadwp_io_num = GPIO_NUM_3,
        .quadhd_io_num = GPIO_NUM_4,
        .max_transfer_sz = 64,
    };

    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return;
    }

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10 * 1000 * 1000,
        .duty_cycle_pos = 128,
        .mode = 0,
        .spics_io_num = GPIO_NUM_6,
        .queue_size = 3,
        .flags = SPI_DEVICE_HALFDUPLEX,
    };

    spi_device_handle_t handle;
    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
        spi_bus_free(SPI2_HOST);
        return;
    }

    uint8_t tx_buffer[10];
    for (int i = 0; i < sizeof(tx_buffer); i++) {
        tx_buffer[i] = i & 0xFF;
    }

    spi_transaction_t qspi_trans = {
        .length = 8 * sizeof(tx_buffer),
        .tx_buffer = tx_buffer,
        .flags = SPI_TRANS_MODE_QIO,
    };

    ret = spi_device_transmit(handle, &qspi_trans);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "QSPI TX failed: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "QSPI TX completed: 64 bytes");
    }



}
