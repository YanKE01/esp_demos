#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "soc/gpio_reg.h"


void app_main(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_NUM_16),
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    uint8_t status = 0;
    uint32_t gpio_mask = (1 << GPIO_NUM_16);

    while (1) {
        status = !status;
        if (status) {
            REG_WRITE(GPIO_OUT_W1TS_REG, gpio_mask);
        } else {
            REG_WRITE(GPIO_OUT_W1TC_REG, gpio_mask);
        }
    }
}
