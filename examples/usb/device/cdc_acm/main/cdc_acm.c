#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tusb.h"
#include "device/usbd.h"
#include "esp_private/usb_phy.h"

static const char *TAG = "CDC_ACM";

static void usb_phy_init(void)
{
    usb_phy_handle_t phy_hdl;
    usb_phy_config_t phy_conf = {
        .controller = USB_PHY_CTRL_OTG,
        .otg_mode = USB_OTG_MODE_DEVICE,
        .target = USB_PHY_TARGET_INT
    };
    usb_new_phy(&phy_conf, &phy_hdl);
}

static void tusb_device_task(void *arg)
{
    while (1) {
        tud_task(); // TinyUSB device task
    }
}

void tud_mount_cb(void)
{
    ESP_LOGI(TAG, "USB device mounted");
}

void tud_umount_cb(void)
{
    ESP_LOGI(TAG, "USB device unmounted");
}

void tud_suspend_cb(bool remote_wakeup_en)
{
    (void)remote_wakeup_en;
    ESP_LOGI(TAG, "USB suspended");
}

void tud_resume_cb(void)
{
    ESP_LOGI(TAG, "USB resumed");
}

void tud_cdc_task(void *arg)
{
    while (1) {
        if (tud_cdc_available()) {
            uint8_t buf[64];
            int len = tud_cdc_read(buf, sizeof(buf));
            if (len > 0) {
                ESP_LOGI(TAG, "Received %d bytes", len);
                tud_cdc_write(buf, len);
                tud_cdc_write_flush();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void tud_cdc_rx_cb(uint8_t itf)
{
    (void) itf;
}

void app_main(void)
{
    usb_phy_init();
    if (!tusb_init()) {
        ESP_LOGE(TAG, "TinyUSB init failed");
        return;
    }
    xTaskCreate(tusb_device_task, "tusb_device_task", 4096, NULL, 5, NULL);
    xTaskCreate(tud_cdc_task, "tud_cdc_task", 5 * 1024, NULL, 5, NULL);
}
