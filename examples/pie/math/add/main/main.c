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

void add_ansi(int16_t *x, int16_t *y, int16_t *z, int n)
{
    for (int i = 0; i < n; i++) {
        z[i] = x[i] + y[i];
    }
}

void add(int16_t *x, int16_t *y, int16_t *z, int n);
void add_pie(int16_t *x, int16_t *y, int16_t *z, int n);


// void add_pie3(int16_t *x, int16_t *y, int16_t *z, int n)
// {
//     // 4*
//     int32_t loop_num = (n / 32) + 1;
//     asm volatile("loopnez %0, loop_end" ::"a"(loop_num));
//     asm volatile("ee.vld.128.ip q0, %0, 16" ::"a"(x));
//     asm volatile("ee.vld.128.ip q1, %0, 16" ::"a"(x));
//     asm volatile("ee.vld.128.ip q2, %0, 16" ::"a"(x));
//     asm volatile("ee.vld.128.ip q3, %0, 16" ::"a"(x));

//     asm volatile("ee.vld.128.ip q4, %0, 16" ::"a"(y));
//     asm volatile("ee.vld.128.ip q5, %0, 16" ::"a"(y));
//     asm volatile("ee.vld.128.ip q6, %0, 16" ::"a"(y));
//     asm volatile("ee.vld.128.ip q7, %0, 16" ::"a"(y));

//     asm volatile("ee.vadds.s16 q0, q0, q4");
//     asm volatile("ee.vadds.s16 q1, q1, q5");
//     asm volatile("ee.vadds.s16 q2, q2, q6");
//     asm volatile("ee.vadds.s16 q3, q3, q7");

//     asm volatile("ee.vst.128.ip q0, %0, 16" ::"a"(z));
//     asm volatile("ee.vst.128.ip q1, %0, 16" ::"a"(z));
//     asm volatile("ee.vst.128.ip q2, %0, 16" ::"a"(z));
//     asm volatile("ee.vst.128.ip q3, %0, 16" ::"a"(z));

//     asm volatile("loop_end:");
// }

void app_pie_4(int16_t *x, int16_t *y, int16_t *z, int n)
{
    asm volatile(
        " addi sp, sp, -32 \n"
        " sw x31, 28(sp) \n"
        " sw x30, 24(sp) \n"
        " sw x29, 20(sp) \n"
        " sw x28, 16(sp) \n"
        " sw x27, 12(sp) \n"
        " mv x31, %0 \n"
        " mv x30, %1 \n"
        " mv x29, %2 \n"
        " mv x28, %3 \n"
        "li x27, 0 \n"
        "loop:\n"
        " beq x27, x28, exit\n"
        " esp.vld.128.ip q0, x31, 16\n"
        " esp.vld.128.ip q1, x31, 16\n"
        " esp.vld.128.ip q2, x31, 16\n"
        " esp.vld.128.ip q3, x31, 16\n"
        " esp.vld.128.ip q4, x30, 16\n"
        " esp.vld.128.ip q5, x30, 16\n"
        " esp.vld.128.ip q6, x30, 16\n"
        " esp.vld.128.ip q7, x30, 16\n"

        " esp.vadd.s16 q0, q0, q4 \n"
        " esp.vadd.s16 q1, q1, q5 \n"
        " esp.vadd.s16 q2, q2, q6 \n"
        " esp.vadd.s16 q3, q3, q7 \n"

        " esp.vst.128.ip q0, x29, 16 \n"
        " esp.vst.128.ip q1, x29, 16 \n"
        " esp.vst.128.ip q2, x29, 16 \n"
        " esp.vst.128.ip q3, x29, 16 \n"

        " addi x27, x27, 32 \n"
        " j loop \n"
        "exit:\n"
        " lw x31, 28(sp) \n"
        " lw x30, 24(sp) \n"
        " lw x29, 20(sp) \n"
        " lw x28, 16(sp) \n"
        " lw x27, 12(sp) \n"
        " addi sp, sp, 32 \n"
        :: "r" (x), "r" (y), "r" (z), "r" (n)
    );
}

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
    for (int i = 0; i < ilength; i++) {
        x[i] = i;
        y[i] = i;
        z[i] = 0;
    }

    // start
    start = esp_cpu_get_cycle_count();
    for (int i = 0; i < iter; i++) {
        add_ansi(x, y, z, ilength);
    }
    end = esp_cpu_get_cycle_count();
    printf("ave Cycles: %lld\n", (end - start) / iter);

    // print result
    for (int i = 0; i < ilength; i++) {
        printf("%d ", z[i]);
    }
    printf("\n");

    // free
    if (x != NULL) {
        dl_lib_free(x);
        x = NULL;
    }
    if (y != NULL) {
        dl_lib_free(y);
        y = NULL;
    }
    if (z != NULL) {
        dl_lib_free(z);
        z = NULL;
    }
}
