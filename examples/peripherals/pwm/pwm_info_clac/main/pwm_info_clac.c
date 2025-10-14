/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_private/esp_clk.h"
#include "driver/mcpwm_cap.h"
#include "driver/gpio.h"

const static char *TAG = "pwm_measure";

// PWM measurement data structure - only raw tick values in ISR
typedef struct {
    uint32_t period_ticks;
    uint32_t high_time_ticks;
} pwm_raw_data_t;

static bool pwm_measure_callback(mcpwm_cap_channel_handle_t cap_chan, const mcpwm_capture_event_data_t *edata, void *user_data)
{
    static uint32_t last_pos_edge = 0;
    static uint32_t last_neg_edge = 0;
    static uint32_t period_start = 0;
    static bool first_pos_edge = true;

    TaskHandle_t task_to_notify = (TaskHandle_t)user_data;
    BaseType_t high_task_wakeup = pdFALSE;
    static pwm_raw_data_t raw_data = {0};

    if (edata->cap_edge == MCPWM_CAP_EDGE_POS) {
        if (first_pos_edge) {
            // First positive edge - just record the timestamp
            period_start = edata->cap_value;
            last_pos_edge = edata->cap_value;
            first_pos_edge = false;
        } else {
            // Calculate period (time between two positive edges) - only integer arithmetic in ISR
            raw_data.period_ticks = edata->cap_value - period_start;
            raw_data.high_time_ticks = last_neg_edge - last_pos_edge;

            // Update for next cycle
            period_start = edata->cap_value;
            last_pos_edge = edata->cap_value;

            // Notify the task with raw tick data - no floating point in ISR
            xTaskNotifyFromISR(task_to_notify, (uint32_t)&raw_data, eSetValueWithOverwrite, &high_task_wakeup);
        }
    } else {
        // Negative edge - record the timestamp
        last_neg_edge = edata->cap_value;
    }

    return high_task_wakeup == pdTRUE;
}


void app_main(void)
{
    ESP_LOGI(TAG, "PWM Frequency and Duty Cycle Measurement Example");
    ESP_LOGI(TAG, "Connect PWM signal to GPIO %d", CONFIG_PWM_CAPTURE_IO);

    ESP_LOGI(TAG, "Install capture timer");
    mcpwm_cap_timer_handle_t cap_timer = NULL;
    mcpwm_capture_timer_config_t cap_conf = {
        .clk_src = MCPWM_CAPTURE_CLK_SRC_DEFAULT,
        .group_id = 0,
    };
    ESP_ERROR_CHECK(mcpwm_new_capture_timer(&cap_conf, &cap_timer));

    ESP_LOGI(TAG, "Install capture channel");
    mcpwm_cap_channel_handle_t cap_chan = NULL;
    mcpwm_capture_channel_config_t cap_ch_conf = {
        .gpio_num = CONFIG_PWM_CAPTURE_IO,
        .prescale = 1,
        // capture on both edges to measure period and duty cycle
        .flags.neg_edge = true,
        .flags.pos_edge = true,
    };
    ESP_ERROR_CHECK(mcpwm_new_capture_channel(cap_timer, &cap_ch_conf, &cap_chan));
    // pull up the GPIO internally
    ESP_ERROR_CHECK(gpio_set_pull_mode(CONFIG_PWM_CAPTURE_IO, GPIO_PULLUP_ONLY));

    ESP_LOGI(TAG, "Register capture callback");
    TaskHandle_t cur_task = xTaskGetCurrentTaskHandle();
    mcpwm_capture_event_callbacks_t cbs = {
        .on_cap = pwm_measure_callback,
    };
    ESP_ERROR_CHECK(mcpwm_capture_channel_register_event_callbacks(cap_chan, &cbs, cur_task));

    ESP_LOGI(TAG, "Enable capture channel");
    ESP_ERROR_CHECK(mcpwm_capture_channel_enable(cap_chan));

    ESP_LOGI(TAG, "Enable and start capture timer");
    ESP_ERROR_CHECK(mcpwm_capture_timer_enable(cap_timer));
    ESP_ERROR_CHECK(mcpwm_capture_timer_start(cap_timer));

    ESP_LOGI(TAG, "Starting PWM measurement...");
    ESP_LOGI(TAG, "Waiting for PWM signal on GPIO %d", CONFIG_PWM_CAPTURE_IO);

    uint32_t notification_value;
    while (1) {
        // wait for PWM measurement data
        if (xTaskNotifyWait(0x00, ULONG_MAX, &notification_value, pdMS_TO_TICKS(2000)) == pdTRUE) {
            pwm_raw_data_t *raw_data = (pwm_raw_data_t *)notification_value;

            // Check for valid measurement (period > 0)
            if (raw_data->period_ticks > 0) {
                // Calculate frequency and duty cycle in main task (floating point operations)
                float period_us = raw_data->period_ticks * (1000000.0 / esp_clk_apb_freq());
                float frequency_hz = 1000000.0 / period_us;
                float duty_cycle_percent = (float)raw_data->high_time_ticks / raw_data->period_ticks * 100.0;
                float high_time_us = raw_data->high_time_ticks * (1000000.0 / esp_clk_apb_freq());

                // Check for reasonable frequency range
                if (frequency_hz > 0 && frequency_hz < 1000000) {
                    ESP_LOGI(TAG, "========================================");
                    ESP_LOGI(TAG, "PWM Measurement:");
                    ESP_LOGI(TAG, "  Frequency: %.2f Hz", frequency_hz);
                    ESP_LOGI(TAG, "  Duty Cycle: %.2f%%", duty_cycle_percent);
                    ESP_LOGI(TAG, "  Period: %.2f us", period_us);
                    ESP_LOGI(TAG, "  High Time: %.2f us", high_time_us);
                    ESP_LOGI(TAG, "========================================");
                } else {
                    ESP_LOGW(TAG, "Invalid frequency calculated: %.2f Hz", frequency_hz);
                }
            } else {
                ESP_LOGW(TAG, "Invalid PWM measurement received (period_ticks = %u)", raw_data->period_ticks);
            }
        } else {
            ESP_LOGW(TAG, "No PWM signal detected on GPIO %d", CONFIG_PWM_CAPTURE_IO);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
