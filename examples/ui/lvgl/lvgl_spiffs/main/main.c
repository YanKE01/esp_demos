#include <stdio.h>
#include "esp_lvgl_port.h"
#include "st7789.h"
#include "esp_check.h"
#include "esp_log.h"
#include "lvgl.h"
#include "ui.h"
#include "hal_spiffs.h"

static const char *TAG = "LVGL_FATFS";
lv_disp_t *lvgl_disp = NULL;
lv_obj_t *lvgl_img = NULL;
lv_obj_t *lvgl_screen = NULL;

lcd_config_t lcd_config = {
    .spi_host_device = SPI3_HOST,
    .dc = GPIO_NUM_4,
    .cs = GPIO_NUM_5,
    .sclk = GPIO_NUM_6,
    .mosi = GPIO_NUM_7,
    .rst = GPIO_NUM_8,
    .backlight = GPIO_NUM_9,
    .lcd_bits_per_pixel = 16,
    .lcd_color_space = LCD_COLOR_SPACE_RGB,
    .lcd_height_res = 240,
    .lcd_vertical_res = 240,
    .lcd_draw_buffer_height = 50,
};

esp_err_t lvgl_init()
{
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,      /* LVGL task priority */
        .task_stack = 8 * 1024,  /* LVGL task stack size */
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
        }};

    lvgl_disp = lvgl_port_add_disp(&disp_cfg);
    return ESP_OK;
}

void app_main(void)
{
    // spiffs init
    ESP_ERROR_CHECK(hal_spiffs_init("/spiffs"));

    // lcd init
    ESP_ERROR_CHECK(lcd_init(lcd_config));
    lcd_fullclean(lcd_panel, lcd_config, rgb565(255, 255, 255));

    // lvgl init
    ESP_ERROR_CHECK(lvgl_init());
    ui_init();

    // add ui
    lv_obj_t *label = lv_label_create(ui_Screen1);
    lv_label_set_text(label, "IMG_Decoder");
    lv_obj_align_to(label, NULL, LV_ALIGN_TOP_MID, 0, 0);

    // add gif
    lv_obj_t *img_gif = lv_gif_create(ui_Screen1);
    lv_gif_set_src(img_gif, "S:/spiffs/example.gif");
    lv_obj_align_to(img_gif, NULL, LV_ALIGN_LEFT_MID, 0, 0);

    // add jpg
    lv_obj_t *img_jpg = lv_img_create(ui_Screen1);
    lv_img_set_src(img_jpg, "S:/spiffs/img_lvgl_logo.jpg");
    lv_obj_align_to(img_jpg, NULL, LV_ALIGN_RIGHT_MID, 0, 0);

    // add qr
    lv_color_t bg_color = lv_palette_lighten(LV_PALETTE_LIGHT_BLUE, 5);
    lv_color_t fg_color = lv_palette_darken(LV_PALETTE_BLUE, 4);
    lv_obj_t *qr = lv_qrcode_create(ui_Screen1, 80, fg_color, bg_color);
    const char *data = "https://lvgl.io";
    lv_qrcode_update(qr, data, strlen(data));
    lv_obj_align_to(qr, NULL, LV_ALIGN_BOTTOM_MID, 0, 0);
}
