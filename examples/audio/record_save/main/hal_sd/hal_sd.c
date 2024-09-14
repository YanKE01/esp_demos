#include "hal_sd.h"
#include "esp_log.h"

static const char *TAG = "SD";
sdmmc_host_t host = SDMMC_HOST_DEFAULT();
sdmmc_card_t *card;

esp_err_t hal_sd_init(hal_sd_spi_config_t config)
{
    esp_err_t ret;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 8 * 1024,
    };

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 1;
    slot_config.clk = config.clk_pin;
    slot_config.cmd = config.cmd_pin;
    slot_config.d0 = config.d0_pin;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    ret = esp_vfs_fat_sdmmc_mount(config.mount_path, &host, &slot_config, &mount_config, &card);

    sdmmc_card_print_info(stdout, card);
    return ret;
}
