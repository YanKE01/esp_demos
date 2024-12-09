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
#include "esp_timer.h"

void *dl_lib_calloc(int cnt, int size, int align)
{
    int total_size = cnt * size + align + sizeof(void *);
    void *res = malloc(total_size);
    if (NULL == res) {
        printf("Item alloc failed. Size: %d = %d x %d + %d + %d\n", total_size, cnt, size, align, sizeof(void *));
        return NULL;
    }
    bzero(res, total_size);
    void **data = (void **)res + 1;
    void **aligned;
    if (align) {
        aligned = (void **)(((size_t)data + (align - 1)) & -align);
    } else {
        aligned = data;
    }

    aligned[-1] = res;
    return (void *)aligned;
}

void rgb565_channel_process(uint16_t *buffer, uint16_t *ouput, size_t n, uint16_t *scale);


void app_main(void)
{
    int ilength = 240 * 240;
    uint16_t scale = 210;  // 缩放比例
    uint16_t *x = dl_lib_calloc(ilength, sizeof(uint16_t), 16);
    uint16_t *y = dl_lib_calloc(ilength, sizeof(uint16_t), 16);

    for (int i = 0; i < ilength; i++) {
        x[i] = 0xAB56;
        y[i] = 0;
    }

    // ansi time
    long long start_time = esp_timer_get_time();
    rgb565_channel_process(x, y, ilength, &scale);
    long long time_instance = esp_timer_get_time() - start_time;

    printf("pie: %llu us \n", time_instance);



    printf("%x\n", y[ilength - 1]);

}
