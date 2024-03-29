#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_private/usb_phy.h"
#include "esp_private/msc_scsi_bot.h"
#include "usb/usb_host.h"
#include "usb/msc_host_vfs.h"
#include "dirent.h"

static usb_phy_handle_t phy_hdl = NULL;
static SemaphoreHandle_t ready_to_deinit_usb;
static const char *TAG = "MSC_HOST";
static QueueHandle_t app_queue;
static msc_host_device_handle_t device;
static msc_host_vfs_handle_t vfs_handle;

static esp_vfs_fat_mount_config_t mount_config = {
    .format_if_mount_failed = false,
    .max_files = 3,
    .allocation_unit_size = 1024,
};

void usb_host_handle_events(void *args)
{
    uint32_t end_flags = 0;
    while (1)
    {
        uint32_t event_flags = 0;
        usb_host_lib_handle_events(portMAX_DELAY, &event_flags); // 等待事件
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS)
        {
            // 所有客户端均被注销
            ESP_LOGI(TAG, "USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS");
            usb_host_device_free_all();
            end_flags |= 1;
        }

        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE)
        {
            ESP_LOGI(TAG, "USB_HOST_LIB_EVENT_FLAGS_ALL_FREE");
            end_flags |= 2;
        }

        if (end_flags == 3)
        {
            xSemaphoreGive(ready_to_deinit_usb);
            break;
        }
    }
    vTaskDelete(NULL);
}

void usb_host_init()
{
    ready_to_deinit_usb = xSemaphoreCreateBinary();

    usb_phy_config_t phy_config = {
        .controller = USB_PHY_CTRL_OTG,
        .target = USB_PHY_TARGET_INT, // USB target is internal PHY
        .otg_mode = USB_OTG_MODE_HOST,
        .otg_speed = USB_PHY_SPEED_UNDEFINED, // in Host mode, the speed is determined by the connected device
    };
    ESP_ERROR_CHECK(usb_new_phy(&phy_config, &phy_hdl));

    usb_host_config_t host_config = {
        .skip_phy_setup = true,             // 手动配置usb phy，允许用户在调用 usb_host_install() 之前手动配置 USB PHY
        .intr_flags = ESP_INTR_FLAG_LEVEL1, // 低优先级
    };
    ESP_ERROR_CHECK(usb_host_install(&host_config));
    xTaskCreatePinnedToCore(usb_host_handle_events, "usb host events", 2 * 2048, NULL, 2, NULL, 0);
}

void usb_host_msc_event_cb(const msc_host_event_t *event, void *arg)
{
    if (event->event == MSC_DEVICE_CONNECTED)
    {
        ESP_LOGI(TAG, "MSC Device Connected"); // 设备连接
    }
    else
    {
        ESP_LOGI(TAG, "MSC Device Disconnected"); // 设备无连接
    }

    // 队列发送事件
    xQueueSend(app_queue, event, 10);
}

void usb_host_msc_init()
{
    ready_to_deinit_usb = xSemaphoreCreateBinary();
    app_queue = xQueueCreate(5, sizeof(msc_host_event_t));
    usb_phy_config_t phy_config = {
        .controller = USB_PHY_CTRL_OTG,
        .target = USB_PHY_TARGET_INT, // 内部Phy
        .otg_mode = USB_OTG_MODE_HOST,
        .otg_speed = USB_PHY_SPEED_UNDEFINED, // 取决于设备速度
    };
    ESP_ERROR_CHECK(usb_new_phy(&phy_config, &phy_hdl));

    const usb_host_config_t host_config = {
        .skip_phy_setup = true,
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };
    ESP_ERROR_CHECK(usb_host_install(&host_config));
    xTaskCreatePinnedToCore(usb_host_handle_events, "usb_events", 2 * 2048, NULL, 2, NULL, 0);

    const msc_host_driver_config_t msc_config = {
        .create_backround_task = true,
        .callback = usb_host_msc_event_cb,
        .stack_size = 4096,
        .task_priority = 5,
    };
    ESP_ERROR_CHECK(msc_host_install(&msc_config));
}

void msc_task(void *args)
{
    while (1)
    {
        // 等待设备连接
        msc_host_event_t msc_event;
        xQueueReceive(app_queue, &msc_event, portMAX_DELAY);
        if (msc_event.event == MSC_DEVICE_CONNECTED)
        {
            // 获取MSC设备地址
            uint8_t device_addr = msc_event.device.address;
            // 初始化MSC设备
            ESP_ERROR_CHECK(msc_host_install_device(device_addr, &device));
            // 挂载虚拟文件系统
            ESP_ERROR_CHECK(msc_host_vfs_register(device, "/usb", &mount_config, &vfs_handle));
            ESP_LOGI(TAG, "USB VFS Done");

            // 扫描所有文件
            DIR *dir = opendir("/usb");
            if (dir != NULL)
            {
                struct dirent *entry;
                while ((entry = readdir(dir)) != NULL)
                {
                    ESP_LOGI(TAG, "%s has file:%s", "/usb", entry->d_name);
                }
            }
            else
            {
                break;
            }
        }
        else
        {
            ESP_ERROR_CHECK(msc_host_vfs_unregister(vfs_handle));
            ESP_ERROR_CHECK(msc_host_uninstall_device(device));
        }
    }

    ESP_ERROR_CHECK(msc_host_vfs_unregister(vfs_handle));
    ESP_ERROR_CHECK(msc_host_uninstall_device(device));
    vTaskDelete(NULL);
}

void app_main(void)
{
    usb_host_msc_init();
    xTaskCreatePinnedToCore(msc_task, "MSC TASK", 2 * 2048, NULL, 5, NULL, 0);
}
