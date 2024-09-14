#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "nvs_flash.h"
#include "st7789.h"
#include "esp_lvgl_port.h"
#include "hal_camera.h"
#include "ui.h"

lv_disp_t *lvgl_disp = NULL;
lv_obj_t *lvgl_img_cam = NULL;
static const char *TAG = "LVGL_CAMERA";

lcd_config_t lcd_config = {
    .spi_host_device = SPI3_HOST,
    .dc = GPIO_NUM_45,
    .cs = GPIO_NUM_14,
    .sclk = GPIO_NUM_21,
    .mosi = GPIO_NUM_47,
    .rst = -1,
    .backlight = GPIO_NUM_48,
    .lcd_bits_per_pixel = 16,
    .lcd_color_space = LCD_COLOR_SPACE_RGB,
    .lcd_height_res = 240,
    .lcd_vertical_res = 240,
    .lcd_draw_buffer_height = 50,
};

lv_img_dsc_t img_dsc = {
    .header.always_zero = 0,
    .header.w = 240,
    .header.h = 240,
    .data_size = 240 * 240 * 2,
    .header.cf = LV_IMG_CF_TRUE_COLOR,
    .data = NULL,
};

esp_err_t lvgl_init()
{
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,      /* LVGL task priority */
        .task_stack = 5 * 1024,  /* LVGL task stack size */
        .task_affinity = 0,      /* LVGL task pinned to core (-1 is no affinity) */
        .task_max_sleep_ms = 10, /* Maximum sleep in LVGL task */
        .timer_period_ms = 5     /* LVGL timer tick period in ms */
    };
    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG, "LVGL port initialization failed");

    // Add lcd screen
    ESP_LOGI(TAG, "Add LCD screen");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = lcd_io,
        .panel_handle = lcd_panel,
        .buffer_size = lcd_config.lcd_height_res * lcd_config.lcd_draw_buffer_height * sizeof(uint16_t),
        .double_buffer = 1,
        .hres = lcd_config.lcd_height_res,
        .vres = lcd_config.lcd_vertical_res,
        .monochrome = false,
        /* Rotation values must be same as used in esp_lcd for initial settings of the screen */
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = true,
        }
    };

    lvgl_disp = lvgl_port_add_disp(&disp_cfg);
    return ESP_OK;
}

void camera_show_task(void *args)
{
    while (1) {
        camera_fb_t *pic = esp_camera_fb_get();
        if (pic == NULL) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
        } else {
            img_dsc.data = pic->buf;
            lv_img_set_src(ui_Image1, &img_dsc);
            esp_camera_fb_return(pic);
        }
    }
}

void app_main(void)
{
    // Init lcd st7789
    ESP_ERROR_CHECK(lcd_init(lcd_config));
    lcd_fullclean(lcd_panel, lcd_config, rgb565(255, 255, 255));

    // Init camera
    ESP_ERROR_CHECK(hal_camera_init());

    // Init lvgl
    ESP_ERROR_CHECK(lvgl_init());
    ui_init();

    // Init camera show task
    xTaskCreatePinnedToCore(&camera_show_task, "camera show", 1024 * 5, NULL, 6, NULL, 1);
}
