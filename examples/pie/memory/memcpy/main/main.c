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

void *dl_lib_calloc(int cnt, int size, int align)
{
    int total_size = cnt * size + align + sizeof(void *);
    void *res = heap_caps_calloc(1, total_size, MALLOC_CAP_INTERNAL);
    // void *res = NULL;
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

void dl_lib_free(void *d)
{
    if (NULL == d) {
        return;
    }

    free(((void **)d)[-1]);
}

void dl_tie728_memcpy(void *dest, void *src, size_t n);

void app_main(void)
{
    int ilength = 3840;
    int iter = 100;
    uint64_t start, end;
    uint16_t *x = dl_lib_calloc(ilength, sizeof(uint16_t), 16);
    uint16_t *y = dl_lib_calloc(ilength, sizeof(uint16_t), 16);

    for (int i = 0; i < ilength; i++) {
        x[i] = i % 65535;
    }

    start = esp_cpu_get_cycle_count();
    for (int i = 0; i < iter; i++) {
        memcpy(y, x, ilength * sizeof(uint16_t));
    }
    end = esp_cpu_get_cycle_count();
    printf("ave Cycles: %lld\n", (end - start) / iter);

    dl_lib_free(x);
    dl_lib_free(y);
}
