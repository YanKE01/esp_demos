#include <stdio.h>
#include <string.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"

void app_main(void)
{
    spi_device_handle_t handle;

    spi_bus_config_t buscfg = {
        .mosi_io_num = GPIO_NUM_1,
        .miso_io_num = GPIO_NUM_2,
        .sclk_io_num = GPIO_NUM_3,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1 * 1000 * 1000,
        .duty_cycle_pos = 128,
        .mode = 0,
        .spics_io_num = GPIO_NUM_4,
        .queue_size = 3,
    };

    spi_bus_add_device(SPI2_HOST, &devcfg, &handle);

    uint8_t sendbuf[4] = {0x01, 0x00, 0x00, 0x00};
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 4 * 8;
    t.tx_buffer = sendbuf;
    spi_device_transmit(handle, &t);
}
