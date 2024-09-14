#pragma once

#include "esp_err.h"


/**
 * @brief app sr init
 *
 * @param model_partition_label
 * @return esp_err_t
 */
esp_err_t app_sr_init(char *model_partition_label);

/**
 * @brief crate app sr task
 *
 */
void app_sr_task_start();
