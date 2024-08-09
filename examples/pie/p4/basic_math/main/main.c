#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_heap_caps.h"
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "dl_calloc.h"
#include "bit_flip.h"

void app_main(void)
{
    int ilength = 2048;
    uint16_t *x_s_rv = dl_lib_calloc(ilength, sizeof(uint16_t), 16);
    uint16_t *y_s_rv = dl_lib_calloc(ilength, sizeof(uint16_t), 16);

    for (int i = 0; i < ilength; i++)
    {
        x_s_rv[i] = i;
    }

    bit_flip_riscv(x_s_rv, y_s_rv, ilength);

    for (int i = 0; i < ilength; i++)
    {
        printf("0x%04x ", y_s_rv[i]);
    }
}
