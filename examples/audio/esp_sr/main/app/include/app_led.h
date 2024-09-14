#pragma once

#include "led_strip.h"

typedef struct {
    bool is_light;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t led_max_num;
    uint8_t led_ctrl_num;
    uint8_t command;
} app_led_config_t;

void app_led_init(gpio_num_t pin, int max_num);
