#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/twai.h"

#define TX_GPIO_NUM (GPIO_NUM_38)
#define RX_GPIO_NUM (GPIO_NUM_39)
#define ID_SLAVE_PING_RESP 0x0B2

static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_100KBITS();
static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL(); /*!< 不做任何滤波，接收所有ID的消息 */
static const twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TX_GPIO_NUM, RX_GPIO_NUM, TWAI_MODE_NORMAL);
static twai_message_t standard_data_message = {.identifier = 0XA2, .extd = 0, .rtr = 0, .data_length_code = 5, .data = {0x12, 0x13, 0x14, 0x15, 0x16}};

void twai_recv_task(void *pvParameters)
{
    while (1) {
        twai_message_t rx_msg;
        twai_receive(&rx_msg, portMAX_DELAY);
        if (rx_msg.identifier == ID_SLAVE_PING_RESP) {
            printf("Received: ID: %lX, DLC: %d\n", rx_msg.identifier, rx_msg.data_length_code);
            for (int i = 0; i < rx_msg.data_length_code; i++) {
                printf("%x ", rx_msg.data[i]);
            }
            printf("\n");
            twai_transmit(&standard_data_message, portMAX_DELAY);
        }
    }

    twai_stop();
    ESP_ERROR_CHECK(twai_driver_uninstall());
    vTaskDelete(NULL);
}

void app_main(void)
{
    // Install Tawi driver
    ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
    ESP_ERROR_CHECK(twai_start());

    xTaskCreate(twai_recv_task, "twai recv", 2 * 1024, NULL, 5, NULL);
}
