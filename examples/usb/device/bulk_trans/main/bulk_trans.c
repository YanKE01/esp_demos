#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "tusb.h"
#include "device/usbd.h"
#include "esp_private/usb_phy.h"
#include "usb_descriptors.h"

static const char *TAG = "usb_bulk";

#define MAX_PAYLOAD_SIZE 1024
#define HEADER_SIZE 4

typedef struct __attribute__((packed))
{
    uint16_t total_len;
    uint16_t crc16;
} usb_packet_header_t;

static uint8_t frame_buffer[MAX_PAYLOAD_SIZE];
static uint16_t received_total = 0;
static uint16_t expected_total = 0;
static uint16_t expected_crc16 = 0;
static bool frame_active = false;

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

static uint16_t crc16_ccitt(const uint8_t *data, uint16_t len, uint16_t init)
{
    uint16_t crc = init;
    while (len--) {
        crc ^= (*data++) << 8;
        for (int i = 0; i < 8; i++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

void tud_vendor_rx_cb(uint8_t itf, uint8_t const *buffer, uint16_t bufsize)
{
    while (tud_vendor_n_available(itf)) {
        uint8_t temp_buf[64];
        int read_len = tud_vendor_n_read(itf, temp_buf, sizeof(temp_buf));
        if (read_len <= 0) {
            return;
        }

        if (!frame_active && read_len >= HEADER_SIZE) {
            // Decode header
            usb_packet_header_t *hdr = (usb_packet_header_t *)temp_buf;
            expected_total = hdr->total_len;
            expected_crc16 = hdr->crc16;

            ESP_LOGI(TAG, "Header: total_len=%d, crc16=0x%04X", expected_total, expected_crc16);

            if (expected_total > MAX_PAYLOAD_SIZE) {
                ESP_LOGE(TAG, "Payload too large");
                return;
            }

            frame_active = true;
            received_total = 0;

            // Copy data from first packet (excluding header)
            int data_len = read_len - HEADER_SIZE;
            if (data_len > 0) {
                memcpy(frame_buffer, temp_buf + HEADER_SIZE, data_len);
                received_total = data_len;
            }

        } else if (frame_active) {
            // Concatenate remaining packet data
            int remain = expected_total - received_total;
            int to_copy = (read_len > remain) ? remain : read_len;

            if (to_copy > 0) {
                memcpy(frame_buffer + received_total, temp_buf, to_copy);
                received_total += to_copy;
            }
        }

        // Verify CRC after receiving the entire frame
        if (frame_active && received_total >= expected_total) {
            printf("Frame received (%d bytes), computing CRC...\n", expected_total);

            uint16_t calc_crc = crc16_ccitt(frame_buffer, expected_total, 0x0000);
            if (calc_crc == expected_crc16) {
                printf("CRC OK (0x%04X)\n", calc_crc);
            } else {
                printf("CRC MISMATCH! expected 0x%04X, got 0x%04X\n", expected_crc16, calc_crc);
                return;
            }

            // Optional: print payload
            for (int i = 0; i < expected_total; i++) {
                printf("%02X ", frame_buffer[i]);
                if ((i + 1) % 10 == 0) {
                    printf("\n");
                }
            }
            if (expected_total % 10 != 0) {
                printf("\n");
            }

            // Send Response
            int response_len = 640;
            uint8_t *response_buf = malloc(sizeof(usb_packet_header_t) + response_len);
            if (!response_buf) {
                ESP_LOGE(TAG, "Failed to allocate response buffer");
                return;
            }

            // Construct response packet
            usb_packet_header_t *response_hdr = (usb_packet_header_t *)response_buf;
            response_hdr->total_len = response_len;

            for (int i = 0; i < response_len; i++) {
                response_buf[sizeof(usb_packet_header_t) + i] = i % 200;
            }
            ESP_LOGI(TAG, "Response last byte: %02X", response_buf[sizeof(usb_packet_header_t) + response_len - 1]);

            response_hdr->crc16 = crc16_ccitt(response_buf + sizeof(*response_hdr), response_len, 0x0000);

            ESP_LOGI(TAG, "Sending response packet, total_len=%d, crc16=0x%04X", response_len, response_hdr->crc16);

            int ret = 0;
            ret = tud_vendor_n_write(0, response_buf, sizeof(usb_packet_header_t) + response_len);
            if (ret < 0) {
                ESP_LOGE(TAG, "USB bulk write failed: %d", ret);
            }

            free(response_buf);

            // Reset statet
            frame_active = false;
            received_total = 0;
            expected_total = 0;
            expected_crc16 = 0;
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
void tud_suspend_cb(bool remote_wakeup_en)
{
    (void)remote_wakeup_en;
    ESP_LOGI(TAG, "USB suspended");
}
void tud_resume_cb(void)
{
    ESP_LOGI(TAG, "USB resumed");
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
