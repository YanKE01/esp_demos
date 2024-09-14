#pragma once

#include "driver/gpio.h"
#include "driver/spi_master.h"

#define WIDTH 128
#define HEIGHT 64

typedef struct {
    union {
        struct {
            gpio_num_t clk;
            gpio_num_t mosi;
            gpio_num_t cs;
            spi_host_device_t spi_host_num;
        } spi;
        struct {
            gpio_num_t scl;
            gpio_num_t sda;
        } i2c;
    } bus;
    gpio_num_t rst;
    gpio_num_t dc;
} ssd1306_hal_config_t;

typedef enum {
    NORMAL = 0, // 正常显示
    NO,         // 不显示
    INVERSE     // 反色显示
} display_mode_t;

typedef struct {
    int16_t start_x;
    int16_t start_y;
    int16_t w;
    int16_t h;
} window_t;

extern uint8_t oled_buffer[HEIGHT / 8][WIDTH];

esp_err_t ssd1306_init(ssd1306_hal_config_t config);
void oled_refersh();
void oled_write_buffer(int16_t x, int16_t y, display_mode_t mode, uint8_t val);
void oled_clear();
void oled_win_draw_vline(window_t *win, int16_t x, int16_t y_start, int16_t y_end);
void oled_win_draw_box(window_t *win, int16_t x_start, int16_t y_start, int16_t width, int16_t height, uint8_t r);
int16_t oled_win_draw_ascii(window_t *win, int16_t x, int16_t y, char c);
void oled_win_draw_str(window_t *win, int16_t x, int16_t y, char *str);
void oled_set_display_mode(display_mode_t mode);
