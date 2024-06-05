#include "hid_keyboard.h"
#include "tinyusb_types.h"
#include "class/hid/hid_device.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef struct
{
    uint8_t modifier;
    uint8_t reserved;
    uint8_t keycode[6];
} key_board_report_t;

const char *hid_keyboard_string_descriptor[5] = {
    // array of pointer to string descriptors
    (char[]){0x09, 0x04}, /*!< support language is english */
    "TinyUSB",            /*!< manufacturer */
    "TinyUSB Device",     /*!< product */
    "123456",             /*!< chip id */
    "Example HID interface",
};

tusb_desc_device_t hid_keyboard_descriptor = {
    .bLength = sizeof(hid_keyboard_descriptor),
    .bDescriptorType = 0x01, // usb2.0 pdf 5.4.1.2
    .bcdUSB = 0x0200,
    .bDeviceClass = 0X00, // 类别在接口描述符中定义，设备描述符中不处理
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

const uint8_t hid_keyboard_report_descriptor[] = {
    // TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(HID_ITF_PROTOCOL_KEYBOARD)),
    HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP),
    HID_USAGE(HID_USAGE_DESKTOP_KEYBOARD),
    HID_COLLECTION(HID_COLLECTION_APPLICATION),

    HID_REPORT_ID(HID_ITF_PROTOCOL_KEYBOARD)

    // Shift / Control / Alt / GUI，左右各2个,一共8个
    HID_USAGE_PAGE(HID_USAGE_PAGE_KEYBOARD),
    HID_USAGE_MIN(224),
    HID_USAGE_MAX(231),
    HID_LOGICAL_MIN(0),
    HID_LOGICAL_MAX(1),
    HID_REPORT_COUNT(8),
    HID_REPORT_SIZE(1),
    HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),

    // 保留字节
    HID_REPORT_COUNT(1),
    HID_REPORT_SIZE(8),
    HID_INPUT(HID_CONSTANT),

    // 6byte的按键
    HID_USAGE_PAGE(HID_USAGE_PAGE_KEYBOARD),
    HID_USAGE_MIN(0),
    HID_USAGE_MAX_N(255, 2),
    HID_LOGICAL_MIN(0),
    HID_LOGICAL_MAX_N(255, 2),
    HID_REPORT_COUNT(6),
    HID_REPORT_SIZE(8),
    HID_INPUT(HID_DATA | HID_ARRAY | HID_ABSOLUTE),
    HID_COLLECTION_END};

const uint8_t hid_keyboard_configuration_descriptor[] = {
    // 配置描述符
    0x09,                                                  /*!< bLength */
    0x02,                                                  /*!< bDescriptorType */
    U16_TO_U8S_LE(9 + 1 * (9 + 9 + 7)),                    /*!< wTotalLength,配置描述符大小+hid接口数*(接口描述符+HID描述符+端点描述符) */
    1,                                                     /*!< bNumInterfaces,接口描述符数量 */
    1,                                                     /*!< bConfigurationValue,配置描述符数量 */
    0,                                                     /*!< iConfiguration,配置描述符索引，只有一个，所以为0 */
    0xA0,                                                  /*!< bmAttributes,总线供电，支持远程唤醒*/
    0x32,                                                  /*!< MaxPower,100MA */

    // 接口描述符
    0x09,                                                  /*!< bDescriptorType */
    0x04,                                                  /*!< bDescriptorType */
    0x00,                                                  /*!< bInterfaceNumber,接口编号，从0开始 */
    0x00,                                                  /*!< bAlternateSetting,备用接口编号 */
    0x01,                                                  /*!< bNumEndpoints,端点数,使用1个端点 */
    0x03,                                                  /*!< bInterfaceClass,HID类 */
    0x01,                                                  /*!< bInterfaceSubClass, 支持启动接口*/
    0x01,                                                  /*!< bInterfaceProtocol,键盘协议 */
    0x00,                                                  /*!< iInterface,接口字符串索引，无 */

    // hid描述符
    0x09,                                                  /*!< HID描述符长度 */
    0X21,                                                  /*!< 描述符类型: HID */
    U16_TO_U8S_LE(0x0100),                                 /*!< HID描述符规范码，1.00  */
    0x21,                                                  /*!< HID描述符：硬件国家码，美国是33,写16进制 */
    0x01,                                                  /*!< HID描述符：类别描述符个数，至少一个 */
    0x22,                                                  /*!< HID描述符：描述符类型，这里是报表 */
    U16_TO_U8S_LE(sizeof(hid_keyboard_report_descriptor)), /*!< HID描述符：报表描述符总长度 */

    // 输入端点描述符
    0x07,                                                  /*!< bLength */
    0x05,                                                  /*!< bDescriptorType */
    0x81,                                                  /*!< bEndpointAddress,1号输入端点 */
    0x03,                                                  /*!< bmAttributes,中断属性 */
    U16_TO_U8S_LE(16),                                     /*!< wMaxPackeSize，最大包长度 */
    10,                                                    /*!< bInterval，端点查询时间*/
};

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    return hid_keyboard_report_descriptor;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;

    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
}

void keyboard_win_test()
{
    uint8_t keycode[6] = {HID_KEY_GUI_LEFT};
    key_board_report_t report;
    memset(&report, 0x00, sizeof(report));
    memcpy(report.keycode, keycode, sizeof(report.keycode));
    tud_hid_report(HID_ITF_PROTOCOL_KEYBOARD, &report, sizeof(report));

    vTaskDelay(pdMS_TO_TICKS(10));
    memset(&report, 0x00, sizeof(report));
    tud_hid_report(HID_ITF_PROTOCOL_KEYBOARD, &report, sizeof(report));
}