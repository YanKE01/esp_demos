#include "app_led.h"
#include "stdio.h"
#include "math.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

led_strip_handle_t led_strip;
app_led_config_t app_led_config;

void app_led_rainbow()
{
    static int count = 0;
    count++;
    app_led_config.g = (uint8_t)(20 * sin(count / 10) + 20);
    app_led_config.r = (uint8_t)(20 * sin((count + 209.3) / 10) + 20);
    app_led_config.b = (uint8_t)(20 * sin((count + 418.7) / 10) + 20);
    count %= 628;
    app_led_config.led_ctrl_num++;
    app_led_config.led_ctrl_num %= app_led_config.led_max_num;
    led_strip_set_pixel(led_strip, app_led_config.led_ctrl_num, app_led_config.r, app_led_config.g, app_led_config.b);
}

void app_led_on()
{
    app_led_config.r = 50;
    app_led_config.g = 40;
    app_led_config.b = 0;
    for (int i = 0; i < app_led_config.led_max_num; i++) {
        led_strip_set_pixel(led_strip, i, app_led_config.r, app_led_config.g, app_led_config.b);
    }
}

void app_led_off()
{
    app_led_config.r = 0;
    app_led_config.g = 0;
    app_led_config.b = 0;
    for (int i = 0; i < app_led_config.led_max_num; i++) {
        led_strip_set_pixel(led_strip, i, app_led_config.r, app_led_config.g, app_led_config.b);
    }
}

void app_led_rand()
{
    app_led_config.r = rand() % 255;
    app_led_config.g = rand() % 255;
    app_led_config.b = rand() % 255;
    for (int i = 0; i < app_led_config.led_max_num; i++) {
        led_strip_set_pixel(led_strip, i, app_led_config.r, app_led_config.g, app_led_config.b);
    }
    app_led_config.command = 6;
}

void app_led_hold()
{
    for (int i = 0; i < app_led_config.led_max_num; i++) {
        led_strip_set_pixel(led_strip, i, app_led_config.r, app_led_config.g, app_led_config.b);
    }
}

void app_led_up_brightness()
{
    app_led_config.r = (app_led_config.r + 10) % 255;
    app_led_config.g = (app_led_config.g + 10) % 255;
    app_led_config.b = (app_led_config.b + 10) % 255;
    for (int i = 0; i < app_led_config.led_max_num; i++) {
        led_strip_set_pixel(led_strip, i, app_led_config.r, app_led_config.g, app_led_config.b);
    }
    app_led_config.command = 8;
}

void app_led_down_brightness()
{
    app_led_config.r = (app_led_config.r >= 10) ? (app_led_config.r - 10) : 0;
    app_led_config.g = (app_led_config.g >= 10) ? (app_led_config.g - 10) : 0;
    app_led_config.b = (app_led_config.b >= 10) ? (app_led_config.b - 10) : 0;
    for (int i = 0; i < app_led_config.led_max_num; i++) {
        led_strip_set_pixel(led_strip, i, app_led_config.r, app_led_config.g, app_led_config.b);
    }
    app_led_config.command = 8;
}

void app_led_clac()
{
    for (int i = 0; i < app_led_config.led_max_num; i++) {
        if (i < 2) {
            led_strip_set_pixel(led_strip, i, app_led_config.r, app_led_config.g, app_led_config.b);
        } else {
            led_strip_set_pixel(led_strip, i, 0, 0, 0);
        }
    }
}

void app_led_good_morning()
{
    static int count = 0;
    count++;
    if (count % 5 == 0) {
        for (int i = 0; i < app_led_config.led_max_num; i++) {
            led_strip_set_pixel(led_strip, i, 0, 0, 0);
        }
    } else {
        for (int i = 0; i < app_led_config.led_max_num; i++) {
            led_strip_set_pixel(led_strip, i, app_led_config.r, app_led_config.g, app_led_config.b);
        }
    }

    if (count == 20) {
        count = 0;
        app_led_config.command = 8;
    }
}

void (*app_led_function[])() = {
    app_led_off,
    app_led_on,
    app_led_rainbow,
    app_led_rand,
    app_led_down_brightness,
    app_led_up_brightness,
    app_led_clac,
    app_led_good_morning,
    app_led_hold,
};

void app_led_task(void *pvParameters)
{
    while (1) {
        (*app_led_function[app_led_config.command])();
        led_strip_refresh(led_strip);
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void app_led_init(gpio_num_t pin, int max_num)
{
    led_strip_config_t strip_config = {
        .strip_gpio_num = pin,                    // The GPIO that connected to the LED strip's data line
        .max_leds = max_num,                      // The number of LEDs in the strip,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB, // Pixel format of your LED strip
        .led_model = LED_MODEL_WS2812,            // LED strip model
        .flags.invert_out = false,                // whether to invert the output signal
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,    // different clock source can lead to different power consumption
        .resolution_hz = 10 * 1000 * 1000, // RMT counter clock frequency
        .flags.with_dma = false,           // DMA feature is available on ESP target like ESP32-S3
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    led_strip_clear(led_strip);
    memset(&app_led_config, 0, sizeof(app_led_config));
    app_led_config.r = 25;
    app_led_config.g = 25;
    app_led_config.b = 22;
    app_led_config.led_max_num = max_num;

    xTaskCreatePinnedToCore(app_led_task, "app_led_task", 2 * 2048, NULL, 3, NULL, 1);
}
