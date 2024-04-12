#include "ssd1306.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "string.h"
#include "font.h"

static const char *TAG = "SSD1306";
uint8_t oled_buffer[HEIGHT / 8][WIDTH] = {0}; // 128*64，传输以字节为单位=8 bit

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
display_mode_t oled_display_mode = NORMAL;

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

        device_config.clock_speed_hz = 20 * 1000 * 1000;
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

void oled_write_buffer(int16_t x, int16_t y, display_mode_t mode, uint8_t val)
{
    if (x > (WIDTH - 1) || y > (HEIGHT / 8 - 1) || x < 0 || y < 0)
    {
        return;
    }
    switch (mode)
    {
    case NORMAL:
        oled_buffer[y][x] |= val;
        break;
    case NO:
        oled_buffer[y][x] &= (~val);
        break;
    case INVERSE:
        oled_buffer[y][x] ^= val;
        break;
    default:
        break;
    }
}

void oled_clear()
{
    memset(oled_buffer, 0, sizeof(oled_buffer));
}

void oled_refersh()
{
    ssd1306_send_buffer(oled_buffer);
}

void oled_win_write_byte(window_t *win, int16_t x, int16_t y, uint8_t val)
{
    uint8_t n = 0, m = 0, temp1 = 0, temp2 = 0;
    if (x > win->w || y > win->h || x < 0 || y <= -7)
    {
        return; // 超出窗口大小
    }

    n = (win->start_y + y) / 8;
    m = (win->start_y + y) % 8;

    // 取出高低八位，用于在不同的page下绘点
    temp1 = val << m;
    temp2 = (val >> (8 - m));

    if (m == 0)
    {
        oled_write_buffer(win->start_x + x, n, oled_display_mode, val); // 恰好是整字节
    }
    else if (m != 0)
    {
        oled_write_buffer(win->start_x + x, n, oled_display_mode, temp1);
        oled_write_buffer(win->start_x + x, n + 1, oled_display_mode, temp2);
    }
}

void oled_win_draw_vline(window_t *win, int16_t x, int16_t y_start, int16_t y_end)
{
    uint8_t n = 0, m = 0;
    if (y_start < 0 && y_end < 0)
    {
        return;
    }

    if (y_start < 0)
    {
        y_start = 0;
    }

    if (y_end > win->h)
    {
        y_end = win->h;
    }

    if (x > win->w || x < 0)
    {
        return;
    }
    if ((y_end - y_start) < 7)
    {
        oled_win_write_byte(win, x, y_start, ~(0xFF << (y_end - y_start)));
    }
    else
    {
        uint8_t i = 0;
        n = (y_end - y_start) / 8;
        m = (y_end - y_start) % 8;
        for (i = 0; i < n; i++)
        {
            oled_win_write_byte(win, x, y_start + i * 8, 0xFF);
        }
        oled_win_write_byte(win, x, y_start + i * 8, ~(0xFF << m));
    }
}

void oled_win_draw_box(window_t *win, int16_t x_start, int16_t y_start, int16_t width, int16_t height, uint8_t r)
{
    uint8_t cir_r = r;
    if ((2 * r > width) || (2 * r > height))
    {
        return;
    }
    for (uint8_t i = 0; i < width; i++)
    {
        oled_win_draw_vline(win, x_start + i, y_start + r, y_start + height - r);
        if (i < cir_r)
            r--;
        if (i >= (width - cir_r - 1))
            r++;
    }
}

int16_t oled_win_draw_ascii(window_t *win, int16_t x, int16_t y, char c)
{
    for (uint8_t i = 0; i < 6; i++)
    {
        oled_win_write_byte(win, x, y, F6x8[c - ' '][i]);
        x++;
        if (x > win->w)
        {
            break; // 超出边界
        }
    }

    return x;
}

void oled_win_draw_str(window_t *win, int16_t x, int16_t y, uint8_t *str)
{
    int16_t cur_x = x; // 当前单个字符位置
    while (*str != '\0')
    {
        cur_x = oled_win_draw_ascii(win, cur_x, y, *str);
        str++;
        if (x > win->w)
        {
            break;
        }
    }
}
