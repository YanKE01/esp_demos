#include <stdlib.h>
#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal_spiffs.h"
#include "unistd.h"
#include "sys/stat.h"
#include <fcntl.h>
#include <dirent.h>
#include "jpeg_decoder.h"
#include "st7789.h"

static const char *TAG = "JPG_DECODE";
const char *filename = "/spiffs/0128.jpg";

lcd_config_t lcd_config = {
    .spi_host_device = SPI3_HOST,
    .dc = GPIO_NUM_4,
    .cs = GPIO_NUM_5,
    .sclk = GPIO_NUM_6,
    .mosi = GPIO_NUM_7,
    .rst = GPIO_NUM_8,
    .lcd_bits_per_pixel = 16,
    .lcd_color_space = LCD_COLOR_SPACE_RGB,
    .lcd_height_res = 240,
    .lcd_vertical_res = 240,
    .lcd_draw_buffer_height = 50,
};

void app_main(void)
{
    ESP_ERROR_CHECK(hal_spiffs_init("/spiffs"));
    ESP_ERROR_CHECK(lcd_init(lcd_config));
    lcd_fullclean(lcd_panel, lcd_config, rgb565(255, 255, 255));

    // read pic
    struct stat st;
    if (stat(filename, &st) == 0)
    {
        ESP_LOGI(TAG, "File %s size is %ld\n", filename, st.st_size);

        uint32_t filesize = (uint32_t)st.st_size; // read pic size
        char *file_buf = heap_caps_malloc(filesize + 1, MALLOC_CAP_DMA);

        if (file_buf == NULL)
        {
            ESP_LOGI(TAG, "Malloc file buffer fail");
            return;
        }

        int f = open(filename, O_RDONLY);

        if (f > 0)
        {
            read(f, file_buf, filesize);
            ESP_LOGI(TAG, "Decode jpg");
            size_t pic_buf_size = 240 * 240 * sizeof(uint16_t);
            uint8_t *pic_buf = heap_caps_malloc(pic_buf_size + 1, MALLOC_CAP_DMA); // create out image buffer
            esp_jpeg_image_cfg_t jpeg_cfg = {
                .indata = (uint8_t *)file_buf,
                .indata_size = filesize,
                .outbuf = pic_buf,
                .outbuf_size = pic_buf_size,
                .out_format = JPEG_IMAGE_FORMAT_RGB565,
                .out_scale = JPEG_IMAGE_SCALE_0,
                .flags = {
                    .swap_color_bytes = 1,
                },
            };

            // decode jpg
            esp_jpeg_image_output_t outimage;
            esp_jpeg_decode(&jpeg_cfg, &outimage);
            ESP_LOGI(TAG, "%s size: %d x %d", filename, outimage.width, outimage.height);
            esp_lcd_panel_draw_bitmap(lcd_panel, 50, 50, 50 + outimage.height, 50 + outimage.width, pic_buf); // write to lcd
            free(pic_buf);
            close(f);
        }

        free(file_buf);
    }
    else
    {
        ESP_LOGI(TAG, "Read Size Fail");
    }
}
