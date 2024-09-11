#include <stdio.h>
#include "stdint.h"
#include "string.h"

int32_t multiply_add(int16_t *x, int16_t *y, int32_t len)
{
    int32_t i;
    int64_t result = 0;
    for (i = 0; i < len; i++) {
        result += (*x) * (*y++);
        x++;
    }

    return result;
}

int32_t multiply_add_pie(int16_t *x, int16_t *y, int32_t len)
{
    int32_t loop_num = (len >> 3) + 1; // 按照8个为一组，为了避免不是8的倍数，所以需要+1
    int32_t outval = 0;                // 存储输出结果
    int32_t shift = 0;

    asm volatile("ee.zero.accx"); // 清空累加寄存器

    asm volatile("loopnez %0, fir_loop_end" ::"a"(loop_num)); // 直接紧跟 loopnez 的目标标签是 fir_loop_end

    asm volatile("ee.vld.128.ip q0, %0, 16" ::"a"(x)); // 加载 128 位的 x 数据到 q0
    asm volatile("ee.vld.128.ip q1, %0, 16" ::"a"(y)); // 加载 128 位的 y 数据到 q1

    // 执行乘法并累加结果到累加器
    asm volatile("ee.vmulas.s16.accx.ld.ip q2, %0, 0, q0, q1" ::"a"(x)); // 这里的as寄存器是随便填写的，不需要在这里更新地址

    asm volatile("fir_loop_end:"); // 循环结束标志

    // 将累加器的结果右移，并存入 outval
    asm volatile("ee.srs.accx %0, %1, 0" : "=a"(outval) : "a"(shift));
    asm volatile("clamps %0, %0, 15" : "+a"(outval)); // CLAMPS ar, as, 7..22，限制在int16_t的范围内
    return outval;
}

#define LENGTH 13

static __attribute__((aligned(16))) int16_t x_raw[LENGTH];
static __attribute__((aligned(16))) int16_t y_raw[LENGTH];

void app_main(void)
{

    for (int i = 0; i < LENGTH; i++) {
        x_raw[i] = i;
        y_raw[i] = i;
    }

    printf("ANSC: %ld \n", multiply_add(x_raw, y_raw, LENGTH));
    printf("PIE: %ld \n", multiply_add_pie(x_raw, y_raw, LENGTH));
}
