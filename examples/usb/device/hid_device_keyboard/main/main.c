#include <stdio.h>
#include "hid_keyboard.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "USB KEYBOARD";

void app_main(void)
{
    ESP_LOGI(TAG, "usb install");
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = &hid_keyboard_descriptor,
        .string_descriptor = hid_keyboard_string_descriptor,
        .string_descriptor_count = sizeof(hid_keyboard_string_descriptor) / sizeof(hid_keyboard_string_descriptor[0]),
        .external_phy = false,
        .configuration_descriptor = hid_keyboard_configuration_descriptor,
    };
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    while (1)
    {
        keyboard_win_test();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
