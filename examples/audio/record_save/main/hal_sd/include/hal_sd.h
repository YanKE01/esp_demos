#pragma once

#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"
#include "driver/gpio.h"

typedef struct
{
    char *mount_path;
    gpio_num_t clk_pin;
    gpio_num_t cmd_pin;
    gpio_num_t d0_pin;
} hal_sd_spi_config_t;

esp_err_t hal_sd_init(hal_sd_spi_config_t config);

