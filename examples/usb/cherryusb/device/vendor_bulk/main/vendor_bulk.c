#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "usbd_core.h"

#define USBD_VID 0xcafe
#define USBD_PID 0x4001
#define USB_CONFIG_SIZE (9 + (9 + 7 + 7))
#define VENDOR_IN_EP 0X81
#define VENDOR_OUT_EP 0X01

struct usbd_interface intf0;
volatile bool ep_tx_busy_flag = false;

static const uint8_t device_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x0100, 0x01)
};

static const uint8_t config_descriptor[] = {
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 1, 1, 0, 100),
    USB_INTERFACE_DESCRIPTOR_INIT(0, 0, 2, 0xFF, 0x00, 0x00, 4),
    USB_ENDPOINT_DESCRIPTOR_INIT(VENDOR_IN_EP, USB_ENDPOINT_TYPE_BULK, 64, 0),
    USB_ENDPOINT_DESCRIPTOR_INIT(VENDOR_OUT_EP, USB_ENDPOINT_TYPE_BULK, 64, 0),
};

static const char *string_descriptors[] = {
    (const char[]){0x09, 0x04}, /* Langid */
    "CherryUSB",                /* Manufacturer */
    "CherryUSB Vendor DEMO",    /* Product */
    "2022123456",               /* Serial Number */
    "Vendor",                   /* Vendor Interface */
};

static const uint8_t *device_descriptor_callback(uint8_t speed)
{
    return device_descriptor;
}

static const uint8_t *config_descriptor_callback(uint8_t speed)
{
    return config_descriptor;
}

static const char *string_descriptor_callback(uint8_t speed, uint8_t index)
{
    if (index > 4) {
        return NULL;
    }
    return string_descriptors[index];
}

const struct usb_descriptor vendor_bulk = {
    .device_descriptor_callback = device_descriptor_callback,
    .config_descriptor_callback = config_descriptor_callback,
    .string_descriptor_callback = string_descriptor_callback,
};

struct usbd_interface *usbd_vendor_init_intf(uint8_t busid, struct usbd_interface *intf)
{
    intf->class_interface_handler = NULL;
    intf->class_endpoint_handler = NULL;
    intf->vendor_handler = NULL;
    intf->notify_handler = NULL;

    return intf;
}

USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t read_buffer[2048]; /* 2048 is only for test speed , please use CDC_MAX_MPS for common*/
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t write_buffer[2048];

void usbd_event_handler(uint8_t busid, uint8_t event)
{
    switch (event) {
    case USBD_EVENT_RESET:
        break;
    case USBD_EVENT_CONNECTED:
        break;
    case USBD_EVENT_DISCONNECTED:
        break;
    case USBD_EVENT_RESUME:
        break;
    case USBD_EVENT_SUSPEND:
        break;
    case USBD_EVENT_CONFIGURED:
        ep_tx_busy_flag = false;
        usbd_ep_start_read(busid, VENDOR_OUT_EP, read_buffer, 2048);
        break;
    case USBD_EVENT_SET_REMOTE_WAKEUP:
        break;
    case USBD_EVENT_CLR_REMOTE_WAKEUP:
        break;

    default:
        break;
    }
}

void usbd_printer_bulk_out(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    USB_LOG_RAW("actual out len:%d\r\n", (unsigned int)nbytes);
    usbd_ep_start_read(busid, VENDOR_OUT_EP, read_buffer, 2048);
}

void usbd_printer_bulk_in(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    USB_LOG_RAW("IN transfer completed, sent %d bytes\n", nbytes);

    if ((nbytes % usbd_get_ep_mps(busid, ep)) == 0 && nbytes) {
        /* send zlp */
        usbd_ep_start_write(busid, VENDOR_IN_EP, NULL, 0);
    } else {
        ep_tx_busy_flag = false;
    }
}

void app_main(void)
{
    for (int i = 0; i < sizeof(write_buffer); i++) {
        write_buffer[i] = i % 256;
    }

    usbd_desc_register(0, &vendor_bulk);
    usbd_add_interface(0, &intf0);

    struct usbd_endpoint vendor_out_ep = {
        .ep_addr = VENDOR_OUT_EP,
        .ep_cb = usbd_printer_bulk_out,
    };

    struct usbd_endpoint vendor_in_ep = {
        .ep_addr = VENDOR_IN_EP,
        .ep_cb = usbd_printer_bulk_in,
    };

    usbd_add_endpoint(0, &vendor_out_ep);
    usbd_add_endpoint(0, &vendor_in_ep);
    usbd_initialize(0, ESP_USBD_BASE, usbd_event_handler);

    while (1) {
        ep_tx_busy_flag = true;
        usbd_ep_start_write(0, VENDOR_IN_EP, write_buffer, 1024);
        while (ep_tx_busy_flag) {
            vTaskDelay(1 / portTICK_PERIOD_MS);
        }
    }
}
