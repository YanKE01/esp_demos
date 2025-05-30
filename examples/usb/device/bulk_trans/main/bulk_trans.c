#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "tusb.h"
#include "device/usbd.h"
#include "esp_private/usb_phy.h"
#include "usb_descriptors.h"

static const char *TAG = "usb_bulk";
#define CONFIG_USB_VENDOR_RX_BUFSIZE  VENDOR_BUF_SIZE

static uint8_t rx_buffer[CONFIG_USB_VENDOR_RX_BUFSIZE];

static void usb_phy_init(void)
{
    usb_phy_handle_t phy_hdl;
    // Configure USB PHY
    usb_phy_config_t phy_conf = {
        .controller = USB_PHY_CTRL_OTG,
        .otg_mode = USB_OTG_MODE_DEVICE,
    };
    phy_conf.target = USB_PHY_TARGET_INT;
    usb_new_phy(&phy_conf, &phy_hdl);
}

static void tusb_device_task(void *arg)
{
    while (1) {
        tud_task();
    }
}

void tud_vendor_rx_cb(uint8_t itf, uint8_t const *buffer, uint16_t bufsize)
{
    while (tud_vendor_n_available(itf)) {
        int read_res = tud_vendor_n_read(itf, rx_buffer, CONFIG_USB_VENDOR_RX_BUFSIZE);
        if (read_res > 0) {
            ESP_LOGI(TAG, "RX %d bytes: %.*s", read_res, read_res, rx_buffer);

            // Echo back the received data
            if (tud_vendor_n_write(itf, rx_buffer, read_res)) {
                tud_vendor_n_flush(itf);
                ESP_LOGI(TAG, "Echo back %d bytes", read_res);
            }
        }
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

void app_main(void)
{
    esp_err_t ret = ESP_OK;

    usb_phy_init();
    bool usb_init = tusb_init();
    if (!usb_init) {
        ESP_LOGE(TAG, "USB Device Stack Init Fail");
        return;
    }

    xTaskCreate(tusb_device_task, "tusb_device_task", 4096, NULL, 5, NULL);
}
