#include "ssd1306.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "string.h"

static const char *TAG = "SSD1306";
uint8_t oled_buffer[HEIGHT / 8][WIDTH] = {0}; // 128*64

typedef struct
{
    uint8_t is_spi;
    union
    {
        struct
        {
            spi_device_handle_t handle;
        } spi;
    } bus;
    gpio_num_t dc;
} ssd1306_device_t;

ssd1306_device_t ssd1306_device;

bool spi_master_write_byte(spi_device_handle_t spi_handle, const uint8_t *data, size_t length)
{
    spi_transaction_t t;
    if (length > 0)
    {
        memset(&t, 0, sizeof(spi_transaction_t));
        t.tx_buffer = data;
        t.length = length * 8; // bits
        spi_device_transmit(spi_handle, &t);
        return true;
    }

    return false;
}

bool spi_master_write_command(ssd1306_device_t *dev, uint8_t command)
{
    static uint8_t command_byte = 0;
    command_byte = command;
    gpio_set_level(dev->dc, 0);
    return spi_master_write_byte(dev->bus.spi.handle, &command_byte, 1);
}

bool spi_master_write_data(ssd1306_device_t *dev, const uint8_t *data, size_t length)
{
    gpio_set_level(dev->dc, 1);
    return spi_master_write_byte(dev->bus.spi.handle, data, length);
}

void ssd1306_send_buffer(uint8_t data[8][128])
{

    for (int i = 0; i < 8; i++)
    {
        if (ssd1306_device.is_spi)
        {
            // Page Addr: 0xB0~0XB7
            spi_master_write_command(&ssd1306_device, 0xB0 + i);
            spi_master_write_command(&ssd1306_device, 0x00);
            spi_master_write_command(&ssd1306_device, 0x10);
            spi_master_write_data(&ssd1306_device, data[i], 128);
        }
    }
}

esp_err_t ssd1306_init(ssd1306_hal_config_t config)
{
    esp_err_t ret = ESP_OK;
    // Init dc
    if (config.dc >= 0)
    {
        gpio_reset_pin(config.dc);
        gpio_set_direction(config.dc, GPIO_MODE_OUTPUT);
        gpio_set_level(config.dc, 0);
    }

    // Init rst
    if (config.rst >= 0)
    {
        gpio_reset_pin(config.rst);
        gpio_set_direction(config.rst, GPIO_MODE_OUTPUT);
        gpio_set_level(config.rst, 0);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_set_level(config.rst, 1);
    }

    if (config.bus.spi.clk != 0)
    {
        ssd1306_device.is_spi = 1;
        // Init Spi
        ESP_LOGI(TAG, "using spi");
        spi_bus_config_t spi_bus_config = {
            .mosi_io_num = config.bus.spi.mosi,
            .miso_io_num = -1,
            .sclk_io_num = config.bus.spi.clk,
            .quadhd_io_num = -1,
            .quadwp_io_num = -1,
            .max_transfer_sz = 0,
        };

        ret = spi_bus_initialize(config.bus.spi.spi_host_num, &spi_bus_config, SPI_DMA_CH_AUTO);
        assert(ret == ESP_OK);

        spi_device_interface_config_t device_config;
        memset(&device_config, 0, sizeof(spi_device_interface_config_t));

        device_config.clock_speed_hz = 5 * 1000 * 1000;
        device_config.spics_io_num = config.bus.spi.cs;
        device_config.queue_size = 1;

        ret = spi_bus_add_device(config.bus.spi.spi_host_num, &device_config, &ssd1306_device.bus.spi.handle);
        assert(ret == ESP_OK);
        ssd1306_device.dc = config.dc;

        spi_master_write_command(&ssd1306_device, 0xAE); // 关闭显示
        spi_master_write_command(&ssd1306_device, 0x00);
        spi_master_write_command(&ssd1306_device, 0x10);
        spi_master_write_command(&ssd1306_device, 0x40);
        spi_master_write_command(&ssd1306_device, 0x81); // 设置对比度
        spi_master_write_command(&ssd1306_device, 0xFF); // 1~255,越大越亮
        spi_master_write_command(&ssd1306_device, 0xA1);
        spi_master_write_command(&ssd1306_device, 0xC8);
        spi_master_write_command(&ssd1306_device, 0xA6);
        spi_master_write_command(&ssd1306_device, 0xA8);
        spi_master_write_command(&ssd1306_device, 0x3F);
        spi_master_write_command(&ssd1306_device, 0xD3);
        spi_master_write_command(&ssd1306_device, 0x00);
        spi_master_write_command(&ssd1306_device, 0xD5);
        spi_master_write_command(&ssd1306_device, 0x80);
        spi_master_write_command(&ssd1306_device, 0xD9); // 设置预充电周期
        spi_master_write_command(&ssd1306_device, 0xF1); // [3:0],PHASE 1;[7:4],PHASE 2;
        spi_master_write_command(&ssd1306_device, 0xDA); // 设置COM硬件引脚配置
        spi_master_write_command(&ssd1306_device, 0x12); // [5:4]配置
        spi_master_write_command(&ssd1306_device, 0xDB);
        spi_master_write_command(&ssd1306_device, 0x40);
        spi_master_write_command(&ssd1306_device, 0x20);
        spi_master_write_command(&ssd1306_device, 0x02);
        spi_master_write_command(&ssd1306_device, 0x8D);
        spi_master_write_command(&ssd1306_device, 0x14);
        spi_master_write_command(&ssd1306_device, 0xA4);
        spi_master_write_command(&ssd1306_device, 0xA6);
        spi_master_write_command(&ssd1306_device, 0xAF); // 开启显示

        ssd1306_send_buffer(oled_buffer); // 清屏
    }
    else if (config.bus.i2c.scl != 0)
    {
    }
    return ret;
}

void oled_write_byte(int x, int y, uint8_t val, display_mode_t mode)
{
    if (x > WIDTH - 1 || y > (HEIGHT / 8 - 1) || x < 0 || y < 0)
    {
        return;
    }

    switch (mode)
    {
    case NORMAL:
        oled_buffer[y][x] |= val;
        break;
    case NO:
        oled_buffer[y][x] &= ~val;
        break;
    case INVERSE:
        oled_buffer[y][x] ^= val;
        break;
    default:
        break;
    }
}


void oled_refersh()
{
    ssd1306_send_buffer(oled_buffer);
}
