#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "tusb.h"
#include "device/usbd.h"
#include "esp_private/usb_phy.h"
#include "usb_descriptors.h"

static const char *TAG = "WIN_USB";

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
        tud_task();
    }
}

/* Handle vendor control request: Windows sends this to get MS OS 2.0 descriptor for WinUSB */
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{
    if (stage != CONTROL_STAGE_SETUP) {
        return true;
    }
    if (request->bmRequestType_bit.type != TUSB_REQ_TYPE_VENDOR) {
        return false;
    }
    /* GET_MS_OS_20_DESCRIPTOR_SET: wIndex 0x0007 per MS spec (some hosts use (iface<<8)|7) */
    if (request->bRequest == VENDOR_REQUEST_MICROSOFT && (request->wIndex == 0x0007 || (request->wIndex & 0xFF) == 0x07)) {
        uint16_t total_len;
        memcpy(&total_len, desc_ms_os_20 + 8, 2);
        return tud_control_xfer(rhport, request, (void *)(uintptr_t)desc_ms_os_20, total_len);
    }
    return false;
}

/* Bulk OUT: read data from host and echo back on bulk IN */
void tud_vendor_rx_cb(uint8_t itf, uint8_t const *buffer, uint16_t bufsize)
{
    (void)buffer;
    (void)bufsize;
    while (tud_vendor_n_available(itf)) {
        uint8_t buf[256];
        int n = tud_vendor_n_read(itf, buf, sizeof(buf));
        if (n <= 0) {
            break;
        }
        ESP_LOGI(TAG, "WinUSB received %d bytes", n);
        for (int i = 0; i < n; i++) {
            printf("%02X ", buf[i]);
        }
        printf("\n");
        tud_vendor_n_write(itf, buf, (size_t)n);
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
    ESP_LOGI(TAG, "USB device suspended");
}

void tud_resume_cb(void)
{
    ESP_LOGI(TAG, "USB device resumed");
}

void app_main(void)
{
    usb_phy_init();
    if (!tusb_init()) {
        ESP_LOGE(TAG, "TinyUSB init failed");
        return;
    }
    xTaskCreate(tusb_device_task, "tusb_device_task", 4096, NULL, 5, NULL);
}
