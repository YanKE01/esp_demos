#include <stdlib.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "sys/types.h"
#include "stdbool.h"
#include "tinyusb.h"
#include "esp_log.h"
#include "hid_audio_ctrl.h"
#include "iot_button.h"

static const char *TAG = "HID AUDIO CTRL";
static button_handle_t boot_btn_handle = NULL;

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    return hid_device_audio_ctrl_report_descriptor;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;

    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
}

void boot_btn_pressdown_cb(void *button_handle, void *usr_data)
{
    ESP_LOGI(TAG, "Report result:%d", hid_device_audio_ctrl());
}

void app_main(void)
{
    ESP_LOGI(TAG, "USB initialization");

    button_config_t cfg = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = 1000,
        .short_press_time = 300,
        .gpio_button_config = {
            .gpio_num = 0,
            .active_level = 0,
        },
    };
    boot_btn_handle = iot_button_create(&cfg);
    iot_button_register_cb(boot_btn_handle, BUTTON_PRESS_DOWN, boot_btn_pressdown_cb, NULL);

    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = &hid_aduio_device_descriptor,
        .string_descriptor = hid_device_audio_string_descriptor,
        .string_descriptor_count = sizeof(hid_device_audio_string_descriptor) / sizeof(hid_device_audio_string_descriptor[0]),
        .external_phy = false,
        .configuration_descriptor = hid_device_audio_configuration_descriptor,
    };
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
}
