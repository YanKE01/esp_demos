#include "cdc_config.h"
#include "string.h"

const char *usb_cdc_string_descriptor[] = {
    // array of pointer to string descriptors
    (char[]){0x09, 0x04}, /*!< support language is english */
    "TinyUSB",            /*!< manufacturer */
    "TinyUSB Device",     /*!< product */
    "123456",             /*!< chip id */
    "Example CDC",
};

const tusb_desc_device_t usb_cdc_device_descriptor = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = 0X01,    // Device
    .bcdUSB = 0x0200,           // Usb 2.0
    .bDeviceClass = 0x02,       // Communications and CDC Control
    .bDeviceSubClass = 0x00,    // None
    .bDeviceProtocol = 0x00,    // None
    .bMaxPacketSize0 = 64,      // 端点0最大包大小
    .idVendor = 0x303A,         // 厂商编号
    .idProduct = 0x4002,        // 产品编号
    .bcdDevice = 0x100,         // 出厂编号
    .iManufacturer = 0x01,      // 厂商字符串索引
    .iProduct = 0x02,           // 产品字符串索引
    .iSerialNumber = 0x03,      // 序列号字符串索引
    .bNumConfigurations = 0x01, // 配置描述符数量为1
};
