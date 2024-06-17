#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/queue.h>
#include <sys/param.h>
#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "usb/usb_host.h"
#include "usb/hid_host.h"

static const char *TAG = "DP100";

QueueHandle_t app_event_queue = NULL;

typedef enum
{
    APP_EVENT = 0,
    APP_EVENT_HID_HOST
} app_event_group_t;

typedef struct
{
    app_event_group_t event_group;
    /* HID Host - Device related info */
    struct
    {
        hid_host_device_handle_t handle;
        hid_host_driver_event_t event;
        void *arg;
    } hid_host_device;
} app_event_queue_t;

static void usb_lib_task(void *arg)
{
    const usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };

    ESP_ERROR_CHECK(usb_host_install(&host_config));
    xTaskNotifyGive(arg);

    while (true)
    {
        uint32_t event_flags;
        usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
        // In this example, there is only one client registered
        // So, once we deregister the client, this call must succeed with ESP_OK
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS)
        {
            ESP_ERROR_CHECK(usb_host_device_free_all());
            break;
        }
    }

    ESP_LOGI(TAG, "USB shutdown");
    // Clean up USB Host
    vTaskDelay(10); // Short delay to allow clients clean-up
    ESP_ERROR_CHECK(usb_host_uninstall());
    vTaskDelete(NULL);
}

void hid_host_device_callback(hid_host_device_handle_t hid_device_handle, const hid_host_driver_event_t event, void *arg)
{
    const app_event_queue_t evt_queue = {
        .event_group = APP_EVENT_HID_HOST,
        // HID Host Device related info
        .hid_host_device.handle = hid_device_handle,
        .hid_host_device.event = event,
        .hid_host_device.arg = arg};

    if (app_event_queue)
    {
        xQueueSend(app_event_queue, &evt_queue, 0); // 发送给main
    }
}

void hid_host_test_interface_callback(hid_host_device_handle_t hid_device_handle,
                                      const hid_host_interface_event_t event,
                                      void *arg)
{
    uint8_t data[64] = {0};
    size_t data_length = 0;
    hid_host_dev_params_t dev_params;
    ESP_ERROR_CHECK(hid_host_device_get_params(hid_device_handle, &dev_params));

    switch (event)
    {
    case HID_HOST_INTERFACE_EVENT_INPUT_REPORT:
        printf("USB port %d, Interface num %d: ",
               dev_params.addr,
               dev_params.iface_num);

        hid_host_device_get_raw_input_report_data(hid_device_handle,
                                                  data,
                                                  64,
                                                  &data_length);

        for (int i = 0; i < data_length; i++)
        {
            printf("%02x ", data[i]);
        }
        printf("\n");
        break;
    case HID_HOST_INTERFACE_EVENT_DISCONNECTED:
        printf("USB port %d, iface num %d removed\n",
               dev_params.addr,
               dev_params.iface_num);
        ESP_ERROR_CHECK(hid_host_device_close(hid_device_handle));
        break;
    case HID_HOST_INTERFACE_EVENT_TRANSFER_ERROR:
        printf("USB Host transfer error\n");
        break;
    default:
        ESP_LOGI(TAG, "HID Interface unhandled event");
        break;
    }
}

void hid_host_device_event(hid_host_device_handle_t hid_device_handle, const hid_host_driver_event_t event, void *arg)
{
    uint8_t *test_buffer = NULL; // for report descriptor
    unsigned int test_length = 0;
    uint8_t tmp[100] = {0}; // for input report
    size_t rep_len = 0;

    hid_host_dev_params_t dev_params;
    ESP_ERROR_CHECK(hid_host_device_get_params(hid_device_handle, &dev_params));

    switch (event)
    {
    case HID_HOST_DRIVER_EVENT_CONNECTED:
        ESP_LOGI(TAG, "HID Device CONNECTED,usb port:%d,interface num:%d", dev_params.addr, dev_params.iface_num);
        const hid_host_device_config_t dev_config = {
            .callback = hid_host_test_interface_callback,
            .callback_arg = NULL};
        ESP_ERROR_CHECK(hid_host_device_open(hid_device_handle, &dev_config));
        ESP_ERROR_CHECK(hid_host_device_start(hid_device_handle));

        //  HID Device info
        hid_host_dev_info_t hid_dev_info;
        ESP_ERROR_CHECK(hid_host_get_device_info(hid_device_handle, &hid_dev_info));
        printf("\t VID: 0x%04X\n", hid_dev_info.VID);
        printf("\t PID: 0x%04X\n", hid_dev_info.PID);
        wprintf(L"\t iProduct: %S \n", hid_dev_info.iProduct);
        wprintf(L"\t iManufacturer: %S \n", hid_dev_info.iManufacturer);
        wprintf(L"\t iSerialNumber: %S \n", hid_dev_info.iSerialNumber);

        hid_class_request_set_protocol(hid_device_handle, 0);
        hid_class_request_get_report(hid_device_handle, HID_REPORT_TYPE_FEATURE, 0, tmp, &rep_len);
        break;
    default:
        break;
    }
}

void app_main(void)
{
    app_event_queue_t evt_queue;

    xTaskCreatePinnedToCore(usb_lib_task, "usb_events", 4096, xTaskGetCurrentTaskHandle(), 2, NULL, 0);
    ulTaskNotifyTake(false, 1000); // wait usb lib

    // lib host driver
    const hid_host_driver_config_t hid_host_driver_config = {
        .create_background_task = true,
        .task_priority = 5,
        .stack_size = 4096,
        .core_id = 0,
        .callback = hid_host_device_callback,
        .callback_arg = NULL};
    ESP_ERROR_CHECK(hid_host_install(&hid_host_driver_config));

    app_event_queue = xQueueCreate(10, sizeof(app_event_queue_t));

    ESP_LOGI(TAG, "Waiting for HID Device to be connected");

    // 等待事件

    while (1)
    {
        if (xQueueReceive(app_event_queue, &evt_queue, portMAX_DELAY))
        {
            if (evt_queue.event_group == APP_EVENT_HID_HOST)
            {
                // hid事件
                hid_host_device_event(evt_queue.hid_host_device.handle, evt_queue.hid_host_device.event, evt_queue.hid_host_device.arg);
            }
        }
    }
}
