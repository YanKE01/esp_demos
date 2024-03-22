#include "hid_audio_ctrl.h"
#include "tinyusb_types.h"
#include "class/hid/hid_device.h"

// 配置描述符+HID设备数量*(接口描述符+HID描述符+端点描述符)
#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)

/**
 * @brief examples/usb/hid_device_audio_ctrl/managed_components/espressif__esp_tinyusb/usb_descriptors.c
 *
 */
const char *hid_device_audio_string_descriptor[5] = {
    // array of pointer to string descriptors
    (char[]){0x09, 0x04}, /*!< support language is english */
    "TinyUSB",            /*!< manufacturer */
    "TinyUSB Device",     /*!< product */
    "123456",             /*!< chip id */
    "Example HID interface",
};

/**
 * @brief device descriptor
 *
 */
tusb_desc_device_t hid_aduio_device_descriptor = {
    .bLength = sizeof(hid_aduio_device_descriptor),
    .bDescriptorType = 0x01, // usb2.0 pdf 5.4.1.2
    .bcdUSB = 0x0200,
    .bDeviceClass = 0X00, // 类别在接口描述符中定义
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor = 0x303A,         // 厂商ID espressif
    .idProduct = 0x00,          // 产品ID
    .bcdDevice = 0x00,          // 设备版本号,1.00版本
    .iManufacturer = 0x01,      // 厂商字符串索引，string descriptor中的索引
    .iProduct = 0x02,           // 产品字符串索引
    .iSerialNumber = 0x03,      // 产品序列号索引
    .bNumConfigurations = 0x01, // 配置描述符个数，1个
};

/**
 * @brief 报告描述符
 *
 */
const uint8_t hid_device_audio_ctrl_report_descriptor[] = {
    TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(2)),
};

/**
 * @brief 配置描述符
 *
 */
const uint8_t hid_device_audio_configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(0, 0, false, sizeof(hid_device_audio_ctrl_report_descriptor), 0x81, CFG_TUD_HID_EP_BUFSIZE, 5),
};

bool hid_device_audio_ctrl()
{
    uint16_t data = HID_USAGE_CONSUMER_VOLUME_DECREMENT;
    tud_hid_report(2, &data, sizeof(data));
    vTaskDelay(10 / portTICK_PERIOD_MS);
    data = 0;
    return tud_hid_report(2, &data, 2);
}