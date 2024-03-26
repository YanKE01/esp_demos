#include <stdio.h>
#include "st7789.h"
#include "esp_log.h"
#include "esp_check.h"
#include "string.h"

static const char *TAG = "LCD";

esp_lcd_panel_io_handle_t lcd_io = NULL;
esp_lcd_panel_handle_t lcd_panel = NULL;

esp_err_t lcd_init(lcd_config_t lcd_config)
{
    esp_err_t ret = ESP_OK;
    /*!< backlight */
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << lcd_config.backlight,
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

    ESP_LOGI(TAG, "Initialize SPI bus");
    const spi_bus_config_t buscfg = {
        .sclk_io_num = lcd_config.sclk,
        .mosi_io_num = lcd_config.mosi,
        .miso_io_num = GPIO_NUM_NC,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = lcd_config.lcd_height_res * lcd_config.lcd_draw_buffer_height * sizeof(uint16_t),
    };

    ESP_RETURN_ON_ERROR(spi_bus_initialize(lcd_config.spi_host_device, &buscfg, SPI_DMA_CH_AUTO), TAG, "SPI init failed");

    ESP_LOGI(TAG, "Install panel IO");
    const esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = lcd_config.dc,
        .cs_gpio_num = lcd_config.cs,
        .pclk_hz = 40 * 1000 * 1000,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };

    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)lcd_config.spi_host_device, &io_config, &lcd_io), err, TAG, "New panel IO failed");

    ESP_LOGI(TAG, "Install LCD driver");
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = lcd_config.rst,
        .color_space = lcd_config.lcd_color_space,
        .bits_per_pixel = 16,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_st7789(lcd_io, &panel_config, &lcd_panel), err, TAG, "New panel failed");

    esp_lcd_panel_reset(lcd_panel);
    esp_lcd_panel_init(lcd_panel);
    esp_lcd_panel_mirror(lcd_panel, false, false);
    esp_lcd_panel_disp_on_off(lcd_panel, true);
    esp_lcd_panel_invert_color(lcd_panel, true);

    ESP_ERROR_CHECK(gpio_set_level(lcd_config.backlight, 1));

    return ESP_OK;

err:
    if (lcd_panel)
    {
        esp_lcd_panel_del(lcd_panel);
    }
    if (lcd_io)
    {
        esp_lcd_panel_io_del(lcd_io);
    }
    spi_bus_free(lcd_config.spi_host_device);
    return ret;
}

void lcd_fullclean(esp_lcd_panel_handle_t lcd_pandel, lcd_config_t lcd_config, uint16_t color)
{
    uint16_t *buffer = heap_caps_malloc(lcd_config.lcd_height_res * sizeof(uint16_t), MALLOC_CAP_INTERNAL);

    for (int i = 0; i < lcd_config.lcd_height_res; i++)
    {
        buffer[i] = swap_hex(color);
    }
    for (int i = 0; i < lcd_config.lcd_vertical_res; i++)
    {
        esp_lcd_panel_draw_bitmap(lcd_pandel, 0, i, lcd_config.lcd_height_res + 1, i + 1, buffer);
    }

    heap_caps_free(buffer);
}