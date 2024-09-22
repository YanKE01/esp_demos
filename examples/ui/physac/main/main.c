#include <stdlib.h>
#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "st7789.h"
#include "driver/gpio.h"
#include "iot_button.h"

#define PHYSAC_IMPLEMENTATION
#define PHYSAC_STANDALONE
#define PHYSAC_NO_THREADS
#include "physac.h"

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

static button_handle_t g_btn;

void lcd_draw_line_simple(esp_lcd_panel_handle_t panel_handle, int x0, int y0, int x1, int y1, uint16_t color)
{
    // 计算x和y的差值
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);

    // 决定步长的方向
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;

    int err = dx - dy; // 误差值

    while (true) {
        // 在当前点(x0, y0)绘制颜色
        esp_lcd_panel_draw_bitmap(panel_handle, x0, y0, x0 + 1, y0 + 1, &color);

        // 如果到达终点，则退出循环
        if (x0 == x1 && y0 == y1) {
            break;
        }

        // 计算误差，更新x和y
        int e2 = err * 2;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

static void button_press_down_cb(void *arg, void *data)
{
    Vector2 pos = {120.0, 10.0};
    CreatePhysicsBodyPolygon(pos, 20, 4, 10);
}

void app_main(void)
{
    ESP_ERROR_CHECK(lcd_init(lcd_config));
    lcd_fullclean(lcd_panel, lcd_config, rgb565(0, 0, 0));

    button_config_t cfg = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = CONFIG_BUTTON_LONG_PRESS_TIME_MS,
        .short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME_MS,
        .gpio_button_config = {
            .gpio_num = 0,
            .active_level = 0,
        },
    };
    g_btn = iot_button_create(&cfg);
    iot_button_register_cb(g_btn, BUTTON_PRESS_DOWN, button_press_down_cb, NULL);

    InitPhysics();

    // Create floor rectangle physics body
    PhysicsBody floor = CreatePhysicsBodyRectangle((Vector2) {
        lcd_config.lcd_vertical_res / 2, lcd_config.lcd_height_res
    }, lcd_config.lcd_vertical_res, 100, 10);
    floor->enabled = false; // Disable body state to convert it to static (no dynamics, but collisions)

    // Create obstacle circle physics body
    PhysicsBody circle = CreatePhysicsBodyCircle((Vector2) {
        lcd_config.lcd_vertical_res / 2, lcd_config.lcd_height_res / 2
    }, 45, 10);
    circle->enabled = false; // Disable body state to convert it to static (no dynamics, but collisions)

    while (1) {

        int bodiesCount = GetPhysicsBodiesCount();
        for (int i = bodiesCount - 1; i >= 0; i--) {
            PhysicsBody body = GetPhysicsBody(i);

            if ((body != NULL) && (body->position.y > lcd_config.lcd_height_res * 2)) {
                DestroyPhysicsBody(body);
            }
        }
        lcd_fullclean(lcd_panel, lcd_config, rgb565(0, 0, 0));
        bodiesCount = GetPhysicsBodiesCount();

        for (int i = 0; i < bodiesCount; i++) {
            PhysicsBody body = GetPhysicsBody(i);

            if (body != NULL) {
                int vertexCount = GetPhysicsShapeVerticesCount(i);
                for (int j = 0; j < vertexCount; j++) {
                    // Get physics bodies shape vertices to draw lines
                    // Note: GetPhysicsShapeVertex() already calculates rotation transformations
                    Vector2 vertexA = GetPhysicsShapeVertex(body, j);

                    int jj = (((j + 1) < vertexCount) ? (j + 1) : 0); // Get next vertex or first to close the shape
                    Vector2 vertexB = GetPhysicsShapeVertex(body, jj);

                    lcd_draw_line_simple(lcd_panel, vertexA.x, vertexA.y, vertexB.x, vertexB.y, 0xFFFF);
                }
            }
        }
    }
}
