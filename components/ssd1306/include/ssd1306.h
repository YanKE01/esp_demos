#pragma once

#include "driver/gpio.h"
#include "driver/spi_master.h"

#define WIDTH 128
#define HEIGHT 64

typedef struct
{
    union
    {
        struct
        {
            gpio_num_t clk;
            gpio_num_t mosi;
            gpio_num_t cs;
            spi_host_device_t spi_host_num;
        } spi;
        struct
        {
            gpio_num_t scl;
            gpio_num_t sda;
        } i2c;
    } bus;
    gpio_num_t rst;
    gpio_num_t dc;
} ssd1306_hal_config_t;

typedef enum
{
    NORMAL = 0, // 正常显示
    NO,         // 不显示
    INVERSE     // 反色显示
} display_mode_t;

esp_err_t ssd1306_init(ssd1306_hal_config_t config);
void oled_refersh();
void oled_write_byte(int x, int y, uint8_t val, display_mode_t mode);