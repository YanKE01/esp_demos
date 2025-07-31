#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tusb.h"
#include "device/usbd.h"
#include "esp_private/usb_phy.h"

static const char *TAG = "CDC_ACM";

static volatile bool s_dtr = false;

static void usb_phy_init(void)
{
    usb_phy_handle_t phy_hdl;
    usb_phy_config_t phy_conf = {
        .controller = USB_PHY_CTRL_OTG,
        .otg_mode = USB_OTG_MODE_DEVICE,
        .target = USB_PHY_TARGET_INT,
#if CONFIG_TINYUSB_RHPORT_HS
        .otg_speed = USB_PHY_SPEED_HIGH,
#endif
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

void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
    (void) itf; (void) rts;
    s_dtr = dtr;
    ESP_LOGI(TAG, "DTR=%d", dtr);
}

void tud_cdc_rx_cb(uint8_t itf)
{
    (void) itf;
}

void tud_cdc_task(void *arg)
{
    (void)arg;
    static uint8_t txbuf[6 * 1024];
    static uint8_t rxbuf[6 * 1024];

    memset(txbuf, 0xAA, sizeof(txbuf));

    while (1) {
        if (s_dtr) {
            tud_cdc_write(txbuf, sizeof(txbuf));
            tud_cdc_write_flush();
        } else {
            if (tud_cdc_available()) {
                // tud_cdc_read(rxbuf, sizeof(rxbuf));
                tud_cdc_n_read_flush(0);
            }
        }
    }
}

void app_main(void)
{
    usb_phy_init();
    tusb_init();

    xTaskCreate(tusb_device_task, "tusb_device_task", 4096, NULL, 5, NULL);
    xTaskCreate(tud_cdc_task, "tud_cdc_task", 8 * 1024, NULL, 4, NULL);
}
