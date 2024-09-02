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
    void *res = malloc(total_size);
    // void *res = NULL;
    if (NULL == res)
    {
        printf("Item alloc failed. Size: %d = %d x %d + %d + %d\n", total_size, cnt, size, align, sizeof(void *));
        return NULL;
    }
    bzero(res, total_size);
    void **data = (void **)res + 1;
    void **aligned;
    if (align)
        aligned = (void **)(((size_t)data + (align - 1)) & -align);
    else
        aligned = data;

    aligned[-1] = res;
    return (void *)aligned;
}

void dl_lib_free(void *d)
{
    if (NULL == d)
        return;

    free(((void **)d)[-1]);
}

void max_ansi(int16_t *x, int16_t *y, int16_t *z, int n)
{
    for (int i = 0; i < n; i++)
    {
        z[i] = (x[i] > y[i] ? x[i] : y[i]);
    }
}

void max_pie(int16_t *x, int16_t *y, int16_t *z, int n);

void app_main(void)
{
    uint64_t start;
    uint64_t end;
    int ilength = 2048;
    int iter = 100;

    int16_t *x = dl_lib_calloc(ilength, sizeof(int16_t), 16);
    int16_t *y = dl_lib_calloc(ilength, sizeof(int16_t), 16);
    int16_t *z = dl_lib_calloc(ilength, sizeof(int16_t), 16);

    // init value
    for (int i = 0; i < ilength; i++)
    {
        x[i] = i;
        y[i] = i + 1;
        z[i] = 0;
    }

    // start
    start = esp_cpu_get_cycle_count();
    for (int i = 0; i < iter; i++)
    {
        max_ansi(x, y, z, ilength);
    }
    end = esp_cpu_get_cycle_count();
    printf("ave Cycles: %lld\n", (end - start) / iter);

    // print result
    for (int i = 0; i < ilength; i++)
    {
        printf("%d ", z[i]);
    }
    printf("\n");

    // free
    if (x != NULL)
    {
        dl_lib_free(x);
        x = NULL;
    }
    if (y != NULL)
    {
        dl_lib_free(y);
        y = NULL;
    }
    if (z != NULL)
    {
        dl_lib_free(z);
        z = NULL;
    }
}
