#include <sys/param.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp32_port.h"
#include "esp_loader.h"

static const char *TAG = "serial_flasher";
extern const uint8_t merged_binary_bin_start[] asm("_binary_merged_binary_bin_start");
extern const uint8_t merged_binary_bin_end[] asm("_binary_merged_binary_bin_end");

static const char *get_error_string(const esp_loader_error_t error)
{
    const char *mapping[ESP_LOADER_ERROR_INVALID_RESPONSE + 1] = {
        "NONE", "UNKNOWN", "TIMEOUT", "IMAGE SIZE",
        "INVALID MD5", "INVALID PARAMETER", "INVALID TARGET",
        "UNSUPPORTED CHIP", "UNSUPPORTED FUNCTION", "INVALID RESPONSE"
    };

    assert(error <= ESP_LOADER_ERROR_INVALID_RESPONSE);

    return mapping[error];
}

// Max line size
#define BUF_LEN 128
static uint8_t buf[BUF_LEN] = {0};

void slave_monitor(void *arg)
{
#if (HIGHER_BAUDRATE != 115200)
    uart_flush_input(UART_NUM_1);
    uart_flush(UART_NUM_1);
    uart_set_baudrate(UART_NUM_1, 115200);
#endif
    while (1) {
        int rxBytes = uart_read_bytes(UART_NUM_1, buf, BUF_LEN, 100 / portTICK_PERIOD_MS);
        buf[rxBytes] = '\0';
        printf("%s", buf);
    }
}


void app_main(void)
{
    const loader_esp32_config_t config = {
        .baud_rate = 115200,
        .uart_port = UART_NUM_1,
        .uart_rx_pin = GPIO_NUM_6,
        .uart_tx_pin = GPIO_NUM_5,
        .reset_trigger_pin = GPIO_NUM_54,
        .gpio0_trigger_pin = GPIO_NUM_53,
    };

    if (loader_port_esp32_init(&config) != ESP_LOADER_SUCCESS) {
        ESP_LOGE(TAG, "serial initialization failed.");
        return;
    }

    ESP_LOGI(TAG, "Serial flasher initialized.");

    esp_loader_connect_args_t connect_config = ESP_LOADER_CONNECT_DEFAULT();
    esp_loader_error_t err = esp_loader_connect(&connect_config);
    if (err != ESP_LOADER_SUCCESS) {
        ESP_LOGE(TAG, "Cannot connect to target. Error: %s\n", get_error_string(err));

        if (err == ESP_LOADER_ERROR_TIMEOUT) {
            ESP_LOGE(TAG, "Check if the host and the target are properly connected.\n");
        } else if (err == ESP_LOADER_ERROR_INVALID_TARGET) {
            ESP_LOGE(TAG, "You could be using an unsupported chip, or chip revision.\n");
        } else if (err == ESP_LOADER_ERROR_INVALID_RESPONSE) {
            ESP_LOGE(TAG, "Try lowering the transmission rate or using shorter wires to connect the host and the target.\n");
        }

        return;
    }
    ESP_LOGI(TAG, "Connected to target");

    err = esp_loader_change_transmission_rate(230400);
    if (err  == ESP_LOADER_ERROR_UNSUPPORTED_FUNC) {
        ESP_LOGE(TAG, "ESP8266 does not support change transmission rate command.");
        return;
    } else if (err != ESP_LOADER_SUCCESS) {
        ESP_LOGE(TAG, "Unable to change transmission rate on target.");
        return;
    } else {
        err = loader_port_change_transmission_rate(230400);
        if (err != ESP_LOADER_SUCCESS) {
            ESP_LOGE(TAG, "Unable to change transmission rate.");
            return;
        }
        ESP_LOGI(TAG, "Transmission rate changed.");
    }
    ESP_LOGI(TAG, "Transmission rate changed to 230400");

    // err = esp_loader_flash_erase();
    // if (err != ESP_LOADER_SUCCESS) {
    //     ESP_LOGE(TAG, "Failed to erase flash");
    //     return;
    // }

    err = esp_loader_flash_erase_region(0, 0x1000);
    if (err != ESP_LOADER_SUCCESS) {
        ESP_LOGE(TAG, "Failed to erase flash region");
        return;
    }

    ESP_LOGI(TAG, "Flash erased successfully");

    uint8_t payload[1024];
    const uint8_t *bin_addr = merged_binary_bin_start;
    size_t size = merged_binary_bin_end - merged_binary_bin_start;
    err = esp_loader_flash_start(0x00, size, sizeof(payload));

    if (err != ESP_LOADER_SUCCESS) {
        ESP_LOGE(TAG, "Erasing flash failed with error: %s.\n", get_error_string(err));

        if (err == ESP_LOADER_ERROR_INVALID_PARAM) {
            ESP_LOGE(TAG, "If using Secure Download Mode, double check that the specified "
                     "target flash size is correct.\n");
        }
        return;
    }
    ESP_LOGI(TAG, "Start programming");

    size_t binary_size = size;
    size_t written = 0;

    while (size > 0) {
        size_t to_read = MIN(size, sizeof(payload));
        memcpy(payload, bin_addr, to_read);

        err = esp_loader_flash_write(payload, to_read);
        if (err != ESP_LOADER_SUCCESS) {
            ESP_LOGE(TAG, "Packet could not be written! Error %s.\n", get_error_string(err));
            return;
        }

        size -= to_read;
        bin_addr += to_read;
        written += to_read;

        int progress = (int)(((float)written / binary_size) * 100);
        printf("\rProgress: %d %%", progress);
    };

    ESP_LOGI(TAG, "Finished programming");

    esp_loader_reset_target();

    // Delay for skipping the boot message of the targets
    vTaskDelay(500 / portTICK_PERIOD_MS);

    // Forward slave's serial output
    ESP_LOGI(TAG, "********************************************");
    ESP_LOGI(TAG, "*** Logs below are print from slave .... ***");
    ESP_LOGI(TAG, "********************************************");
    xTaskCreate(slave_monitor, "slave_monitor", 2048, NULL, configMAX_PRIORITIES - 1, NULL);


}
