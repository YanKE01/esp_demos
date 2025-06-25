#include <linux/init.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/crc16.h>
#include <linux/kernel.h>

#define BULK_PACKET_MAX_SIZE 64
#define PROTOCOL_HEADER_SIZE 6
#define PAYLOAD_SIZE (BULK_PACKET_MAX_SIZE - PROTOCOL_HEADER_SIZE)
#define RX_BUFFER_SIZE 1024 // Receive buffer size

struct usb_packet_header {
    uint16_t total_len;
    uint16_t crc16;
} __attribute__((packed));

static const struct usb_device_id id_table[] = {
    {
        .idVendor = 0xcafe,
        .idProduct = 0x4001,
        .match_flags = USB_DEVICE_ID_MATCH_VENDOR | USB_DEVICE_ID_MATCH_PRODUCT,
    },
    {},
};

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

static int esp_usb_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
    pr_info("USB device detected.\n");
    struct usb_host_interface *iface_desc = NULL;
    struct usb_endpoint_descriptor *endpoint = NULL;
    struct usb_device *udev = interface_to_usbdev(interface);

    int in_pipe = 0, out_pipe = 0;

    // find input and oupt endpoint
    iface_desc = interface->cur_altsetting;
    for (int i = 0; i < iface_desc->desc.bNumEndpoints; i++) {
        endpoint = &iface_desc->endpoint[i].desc;

        if (usb_endpoint_is_bulk_out(endpoint)) {
            pr_info("Found bulk OUT endpoint: 0x%02x\n", endpoint->bEndpointAddress);
            out_pipe = usb_sndbulkpipe(udev, endpoint->bEndpointAddress);
        }

        if (usb_endpoint_is_bulk_in(endpoint)) {
            pr_info("Found bulk IN endpoint: 0x%02x\n", endpoint->bEndpointAddress);
            in_pipe = usb_rcvbulkpipe(udev, endpoint->bEndpointAddress);
        }
    }

    // Test sync transfer
    int total_len = 640;
    uint8_t *tx_buf = kzalloc(sizeof(struct usb_packet_header) + total_len, GFP_KERNEL);
    if (!tx_buf) {
        return -ENOMEM;
    }

    // Construct packet header
    struct usb_packet_header *hdr = (struct usb_packet_header *)tx_buf;
    hdr->total_len = cpu_to_le16(total_len);

    // Construct packet data
    for (int i = 0; i < total_len; i++) {
        tx_buf[sizeof(struct usb_packet_header) + i] = i % 255;
    }

    hdr->crc16 = cpu_to_le16(crc16_ccitt(tx_buf + sizeof(*hdr), total_len, 0x0000));

    int actual_length = 0, ret = 0;
    ret = usb_bulk_msg(udev, out_pipe, tx_buf, total_len + sizeof(struct usb_packet_header), &actual_length, 1000);
    if (ret < 0) {
        pr_err("USB bulk write failed: %d\n", ret);
    } else {
        pr_info("Wrote %d bytes to USB bulk endpoint, crc: 0x%04x\n", actual_length, le16_to_cpu(hdr->crc16));
    }

    kfree(tx_buf);

    // Read response from device
    uint8_t *rx_buf = kzalloc(RX_BUFFER_SIZE, GFP_KERNEL);
    if (!rx_buf) {
        pr_err("Failed to allocate receive buffer\n");
        return -ENOMEM;
    }

    int rx_actual_length = 0;
    ret = usb_bulk_msg(udev, in_pipe, rx_buf, RX_BUFFER_SIZE, &rx_actual_length, 2000);
    if (ret < 0) {
        pr_err("USB bulk read failed: %d\n", ret);
        kfree(rx_buf);
        return ret;
    }

    pr_info("Received %d bytes from device\n", rx_actual_length);

    // Decode packet header
    struct usb_packet_header *rx_hdr = (struct usb_packet_header *)rx_buf;
    uint16_t rx_total_len = le16_to_cpu(rx_hdr->total_len);
    uint16_t rx_crc16 = le16_to_cpu(rx_hdr->crc16);

    pr_info("Header: total_len=%d, crc16=0x%04x\n", rx_total_len, rx_crc16);
    pr_info("Expected header size: %zu bytes\n", sizeof(struct usb_packet_header));
    pr_info("Actual received data size: %d bytes\n", rx_actual_length);

    // Verify CRC
    uint16_t calculated_crc = crc16_ccitt(rx_buf + sizeof(struct usb_packet_header), rx_total_len, 0x0000);
    if (calculated_crc != rx_crc16) {
        pr_err("CRC verification failed: expected 0x%04x, got 0x%04x\n", rx_crc16, calculated_crc);
    } else {
        pr_info("CRC verification passed\n");
    }

    // Print received data
    pr_info("Last byte: %02x\n", rx_buf[rx_actual_length - 1]);


    kfree(rx_buf);

    return 0;
}

static void esp_usb_disconnect(struct usb_interface *interface)
{
    pr_info("USB device disconnected.\n");
}

static struct usb_driver esp_usb_driver = {
    .name = "esp_usb_driver",
    .probe = esp_usb_probe,
    .disconnect = esp_usb_disconnect,
    .id_table = id_table,
};

static int __init esp_usb_init(void)
{
    pr_info("ESP_USB module loaded.\n");
    return usb_register(&esp_usb_driver);
}

static void __exit esp_usb_exit(void)
{
    pr_info("ESP_USB module unloaded.\n");
    usb_deregister(&esp_usb_driver);
}

module_init(esp_usb_init);
module_exit(esp_usb_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yanke2@espressif.com");
MODULE_DESCRIPTION("Test USB Bulk Transfer Linux Driver");
MODULE_VERSION("1.0");
