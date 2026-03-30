#include <stdio.h>
#include <string.h>
#include "tusb.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_private/usb_phy.h"
#include "usb_descriptors.h"

static const char *TAG = "VENDOR";

#define STALL_TEST         0
#define STALL_AFTER_BYTES  (64 * 1024)
#define MAX_PAYLOAD_SIZE    CFG_TUD_VENDOR_RX_BUFSIZE
#define TARGET_SIZE         (2 * 1024 * 1024)   // 2MB
#define SUMMARY_BYTES       10
#define VENDOR_OUT_EP_ADDR  EPNUM_VENDOR

void usbd_edpt_stall(uint8_t rhport, uint8_t ep_addr);

static uint8_t usb_rx_temp_buffer[MAX_PAYLOAD_SIZE];
static uint32_t s_total_received = 0;
static uint32_t s_last_report    = 0;
static uint8_t s_first_bytes[SUMMARY_BYTES];
static uint8_t s_last_bytes[SUMMARY_BYTES];
static bool s_stall_triggered = false;

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

void tud_mount_cb(void)
{
    ESP_LOGI(TAG, "USB device mounted");
    s_stall_triggered = false;
}

void tud_umount_cb(void)
{
    ESP_LOGI(TAG, "USB device unmounted");
    s_stall_triggered = false;
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

void tud_vendor_rx_cb(uint8_t itf, uint8_t const *buffer, uint16_t bufsize)
{
    (void)buffer;
    (void)bufsize;

    while (tud_vendor_n_available(itf)) {
        uint32_t count = tud_vendor_n_read(itf, usb_rx_temp_buffer, sizeof(usb_rx_temp_buffer));
        if (count == 0) {
            break;
        }
        if (s_total_received < SUMMARY_BYTES) {
            uint32_t first_copy = SUMMARY_BYTES - s_total_received;
            if (first_copy > count) {
                first_copy = count;
            }
            memcpy(s_first_bytes + s_total_received, usb_rx_temp_buffer, first_copy);
        }

        if (count >= SUMMARY_BYTES) {
            memcpy(s_last_bytes, usb_rx_temp_buffer + count - SUMMARY_BYTES, SUMMARY_BYTES);
        } else {
            memmove(s_last_bytes, s_last_bytes + count, SUMMARY_BYTES - count);
            memcpy(s_last_bytes + SUMMARY_BYTES - count, usb_rx_temp_buffer, count);
        }

        s_total_received += count;

#if STALL_TEST
        if (!s_stall_triggered && s_total_received >= STALL_AFTER_BYTES) {
            usbd_edpt_stall(0, VENDOR_OUT_EP_ADDR);
            s_stall_triggered = true;
            ESP_LOGW(TAG, "STALL_TEST: stalled OUT endpoint 0x%02X after %lu bytes",
                     VENDOR_OUT_EP_ADDR, (unsigned long)s_total_received);
        }
#endif

        // Log every 256KB
        if (s_total_received - s_last_report >= (256 * 1024)) {
            ESP_LOGI(TAG, "Received %lu bytes (%.1f KB)",
                     (unsigned long)s_total_received, s_total_received / 1024.0f);
            s_last_report = s_total_received;
        }

        if (s_total_received >= TARGET_SIZE) {
            ESP_LOGI(TAG, "Done! Total received: %lu bytes (2MB complete)",
                     (unsigned long)s_total_received);
            ESP_LOGI(TAG, "First 10 bytes: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
                     s_first_bytes[0], s_first_bytes[1], s_first_bytes[2], s_first_bytes[3], s_first_bytes[4],
                     s_first_bytes[5], s_first_bytes[6], s_first_bytes[7], s_first_bytes[8], s_first_bytes[9]);
            ESP_LOGI(TAG, "Last 10 bytes: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
                     s_last_bytes[0], s_last_bytes[1], s_last_bytes[2], s_last_bytes[3], s_last_bytes[4],
                     s_last_bytes[5], s_last_bytes[6], s_last_bytes[7], s_last_bytes[8], s_last_bytes[9]);
            s_total_received = 0;
            s_last_report    = 0;
            s_stall_triggered = false;
        }
    }
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
