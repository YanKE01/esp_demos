/*
 * SPDX-FileCopyrightText: 2010-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

/*
 * The following example demonstrates a master node in a TWAI network. The master
 * node is responsible for initiating and stopping the transfer of data messages.
 * The example will execute multiple iterations, with each iteration the master
 * node will do the following:
 * 1) Start the TWAI driver
 * 2) Repeatedly send ping messages until a ping response from slave is received
 * 3) Send start command to slave and receive data messages from slave
 * 4) Send stop command to slave and wait for stop response from slave
 * 5) Stop the TWAI driver
 */
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/twai.h"

/* --------------------- Definitions and static variables ------------------ */
//Example Configuration
#define PING_PERIOD_MS          250
#define NO_OF_DATA_MSGS         50
#define NO_OF_ITERS             3
#define ITER_DELAY_MS           1000
#define RX_TASK_PRIO            8
#define TX_TASK_PRIO            9
#define CTRL_TSK_PRIO           10
#define TX_GPIO_NUM             10
#define RX_GPIO_NUM             11
#define EXAMPLE_TAG             "TWAI Master"

#define ID_MASTER_STOP_CMD      0x0A0
#define ID_MASTER_START_CMD     0x0A1
#define ID_MASTER_PING          0x0A2
#define ID_SLAVE_STOP_RESP      0x0B0
#define ID_SLAVE_DATA           0x0B1
#define ID_SLAVE_PING_RESP      0x0B2

typedef enum {
    TX_SEND_PINGS,
    TX_SEND_START_CMD,
    TX_SEND_DATA,
    TX_SEND_DATA_AFTER_RECOVERD,
    TX_SEND_STOP_CMD,
    TX_TASK_EXIT,
} tx_task_action_t;

static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_100KBITS();
static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

//Set TX queue length to 5 (listen only mode = 0)
static const twai_general_config_t g_config = {.mode = TWAI_MODE_NORMAL,
                                               .tx_io = TX_GPIO_NUM, .rx_io = RX_GPIO_NUM,
                                               .clkout_io = TWAI_IO_UNUSED, .bus_off_io = TWAI_IO_UNUSED,
                                               .tx_queue_len = 5, .rx_queue_len = 5,
                                               .alerts_enabled = TWAI_ALERT_NONE,
                                               .clkout_divider = 0
                                              };

//Data bytes of data message will be initialized in the transmit task
static twai_message_t data_message1 = {.identifier = ID_SLAVE_DATA, .data_length_code = 4,
                                       .data = {0x01, 0x02, 0, 0, 0, 0, 0, 0}
                                      };

static QueueHandle_t tx_task_queue;
static TimerHandle_t timer0;
static TimerHandle_t timer1;

volatile int timer_count = 0;

static void timer0_callback(TimerHandle_t timer)
{
    tx_task_action_t action = TX_SEND_DATA;
    xQueueSend(tx_task_queue, &action, 0);
}

static void timer1_callback(TimerHandle_t timer)
{
    tx_task_action_t action = TX_SEND_DATA;
    xQueueSend(tx_task_queue, &action, 0);
}

static twai_message_t *get_trans_data1()
{
    data_message1.data[0]++;
    return &data_message1;
}

static void twai_transmit_task(void *arg)
{
    twai_message_t *tx_msg;
    twai_status_info_t status_info;
    esp_err_t err;
    while (1) {
        tx_task_action_t action;
        xQueueReceive(tx_task_queue, &action, portMAX_DELAY);
        if (action == TX_SEND_DATA) {
            twai_get_status_info(&status_info);
            ESP_LOGI(EXAMPLE_TAG, "status is=%d, tx_num=%lu, tx_error_count:%lu", status_info.state, status_info.msgs_to_tx, status_info.tx_error_counter);
            if (status_info.state == TWAI_STATE_RUNNING) {
                tx_msg = get_trans_data1();
                err = twai_transmit(tx_msg, pdMS_TO_TICKS(50));
                // Lost arbitration, a timer can be established and scheduled retries can be made.
                if (err != ESP_OK) {
                    ESP_LOGE(EXAMPLE_TAG, "status is=%d, (%s)", status_info.state, esp_err_to_name(err));
                }
            } else if (status_info.state == TWAI_STATE_BUS_OFF) {
                ESP_LOGE(EXAMPLE_TAG, "Bus Off state");
                twai_initiate_recovery();    //Needs 128 occurrences of bus free signal
            } else {
                // failed, you can set a timer to get the twai status, if status is running, then give the TX_SEND_DATA single quickly
                ESP_LOGE(EXAMPLE_TAG, "status is=%d, tx_num=%"PRIu32, status_info.state, status_info.msgs_to_tx);
            }
        } else if (action == TX_TASK_EXIT) {
            ESP_LOGE(EXAMPLE_TAG, "Exit tx task");
            break;
        }
    }
    vTaskDelete(NULL);
}

static void twai_control_task(void *arg)
{
    esp_err_t err;
    twai_message_t rx_msg;
    err = twai_start();
    if (err != ESP_OK) {
        ESP_LOGE(EXAMPLE_TAG, "start failed");
    }
    ESP_LOGI(EXAMPLE_TAG, "Driver started");
    xTimerStart(timer0, 1);

    //Prepare to trigger errors, reconfigure alerts to detect change in error state
    twai_reconfigure_alerts(TWAI_ALERT_ALL, NULL);

    while (1) {
        uint32_t alerts;
        twai_read_alerts(&alerts, portMAX_DELAY);
        ESP_LOGI(EXAMPLE_TAG, "alerts=0x%"PRIx32, alerts);
        if (alerts & TWAI_ALERT_BUS_OFF) {
            ESP_LOGI(EXAMPLE_TAG, "Bus Off state");
            twai_initiate_recovery();    //Needs 128 occurrences of bus free signal
            for (int i = 2; i > 0; i--) {
                ESP_LOGW(EXAMPLE_TAG, "Initiate bus recovery in %d", i);
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        }
        if (alerts & TWAI_ALERT_BUS_RECOVERED) {
            //Bus recovery was successful, exit control task to uninstall driver
            ESP_LOGI(EXAMPLE_TAG, "Bus Recovered");
            err = twai_start();
            if (err != ESP_OK) {
                ESP_LOGE(EXAMPLE_TAG, "%d, err", __LINE__);
            }
            err = twai_reconfigure_alerts(TWAI_ALERT_ALL, &alerts);
            if (err != ESP_OK) {
                ESP_LOGE(EXAMPLE_TAG, "%d, err", __LINE__);
            }
            // re-try trans
            xTimerStart(timer1, 1);
            ESP_LOGW(EXAMPLE_TAG, "%d, err", __LINE__);
        }
        if (alerts & TWAI_ALERT_RX_DATA) {
            err = twai_receive(&rx_msg, pdMS_TO_TICKS(10));
            if (err != ESP_OK) {
                ESP_LOGE(EXAMPLE_TAG, "RX failed");
            } else {
                uint32_t data = 0;
                for (int i = 0; i < rx_msg.data_length_code; i++) {
                    data |= (rx_msg.data[i] << (i * 8));
                }
                // ESP_LOGI(EXAMPLE_TAG, "rx_id=%"PRIu32", Received data value %"PRIu32, rx_msg.identifier, data);
                ESP_LOGI(EXAMPLE_TAG, "rx_id=0x%"PRIx32", Received data value %"PRIu8, rx_msg.identifier, rx_msg.data[0]);
            }
        }
        if (alerts & TWAI_ALERT_ARB_LOST) {
            ESP_LOGW(EXAMPLE_TAG, "apb lost, %d", __LINE__);
            // Todo, start timer to re-try trans
            xTimerStart(timer1, 1);
        }
        if (alerts & TWAI_ALERT_BUS_ERROR) {
            ESP_LOGW(EXAMPLE_TAG, "bus err, %d", __LINE__);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        if (alerts & TWAI_ALERT_RX_QUEUE_FULL) {
            ESP_LOGW(EXAMPLE_TAG, "rx full, %d", __LINE__);
        }
        if (alerts & TWAI_ALERT_PERIPH_RESET) {
            ESP_LOGW(EXAMPLE_TAG, "reset, %d", __LINE__);
        }
    }
    vTaskDelete(NULL);
}

void app_main(void)
{
    tx_task_queue = xQueueCreate(3, sizeof(tx_task_action_t));
    timer0 = xTimerCreate("timer0", 1000 / portTICK_PERIOD_MS, pdTRUE,
                          (void *)&timer_count, timer0_callback);
    timer1 = xTimerCreate("timer1", 500 / portTICK_PERIOD_MS, pdTRUE,
                          (void *)&timer_count, timer1_callback);

    if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
        ESP_LOGI(EXAMPLE_TAG, "Driver install failed");
    }
    ESP_LOGI(EXAMPLE_TAG, "Driver installed");
    xTaskCreatePinnedToCore(twai_transmit_task, "TWAI_tx", 4096, NULL, TX_TASK_PRIO, NULL, 0);
    xTaskCreatePinnedToCore(twai_control_task, "TWAI_ctrl", 4096, NULL, CTRL_TSK_PRIO, NULL, 0);
}
