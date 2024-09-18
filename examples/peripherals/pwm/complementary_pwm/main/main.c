#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/mcpwm_prelude.h"

#define EXAMPLE_FOC_PWM_UH_GPIO 47
#define EXAMPLE_FOC_PWM_UL_GPIO 21
#define EXAMPLE_FOC_PWM_VH_GPIO 14
#define EXAMPLE_FOC_PWM_VL_GPIO 13
#define EXAMPLE_FOC_PWM_WH_GPIO 12
#define EXAMPLE_FOC_PWM_WL_GPIO 11

void app_main(void)
{
    mcpwm_timer_handle_t timer = NULL;
    mcpwm_timer_config_t timer_config = {
        .group_id = 0,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000, /*!< 10MHZ, 1 tick = 0.1us */
        .period_ticks = 500,               /*!< 20Khz */
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP_DOWN,
    };
    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timer));

    // Init operators
    mcpwm_oper_handle_t operators[3] = {NULL};
    mcpwm_operator_config_t operator_config = {
        .group_id = 0, /*!< operator must be in the same group to the timer */
    };
    for (int i = 0; i < 3; i++) {
        ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config, &operators[i]));
        ESP_ERROR_CHECK(mcpwm_operator_connect_timer(operators[i], timer));
    }

    // Init comparators
    mcpwm_cmpr_handle_t comparators[3] = {NULL};
    mcpwm_comparator_config_t comparator_config = {
        .flags.update_cmp_on_tez = true,
    };
    for (int i = 0; i < 3; i++) {
        ESP_ERROR_CHECK(mcpwm_new_comparator(operators[i], &comparator_config, &comparators[i]));
        ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparators[i], 0));
    }

    // Assigning GPIO pins
    int gen_gpios[3][2] = {
        {EXAMPLE_FOC_PWM_UH_GPIO, EXAMPLE_FOC_PWM_UL_GPIO},
        {EXAMPLE_FOC_PWM_VH_GPIO, EXAMPLE_FOC_PWM_VL_GPIO},
        {EXAMPLE_FOC_PWM_WH_GPIO, EXAMPLE_FOC_PWM_WL_GPIO},
    };
    mcpwm_generator_config_t gen_config = {};
    mcpwm_gen_handle_t generators[3][2] = {NULL};
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 2; j++) {
            gen_config.gen_gpio_num = gen_gpios[i][j];
            ESP_ERROR_CHECK(mcpwm_new_generator(operators[i], &gen_config, &generators[i][j]));
        }
    }

    for (int i = 0; i < 3; i++) {
        ESP_ERROR_CHECK(mcpwm_generator_set_actions_on_compare_event(generators[i][0],
                        MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparators[i], MCPWM_GEN_ACTION_LOW),
                        MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_DOWN, comparators[i], MCPWM_GEN_ACTION_HIGH),
                        MCPWM_GEN_COMPARE_EVENT_ACTION_END()));
    }

    // Init inverse
    mcpwm_dead_time_config_t inv_config = {
        .negedge_delay_ticks = 5,
        .flags.invert_output = true,
    };
    for (int i = 0; i < 3; i++) {
        ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(generators[i][0], generators[i][1], &inv_config));
    }

    // Enabale timer
    ESP_ERROR_CHECK(mcpwm_timer_enable(timer));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP));

    // Set compare
    while (1) {
        mcpwm_comparator_set_compare_value(comparators[0], 100); // 50%
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
