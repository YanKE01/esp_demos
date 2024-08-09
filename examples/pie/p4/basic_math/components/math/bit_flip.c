#include "bit_flip.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void bit_flip(uint16_t *x, uint16_t *y, int n)
{
    for (int i = 0; i < n; i++)
    {
        uint16_t current = x[i];
        uint16_t swapped = (current >> 8) | (current << 8);
        y[i] = swapped;
    }
}

void bit_flip_riscv(uint16_t *x, uint16_t *y, int n)
{
    asm volatile(
        "  addi sp, sp, -32 \n " // 为局部变量分配空间
        "  sw x31, 28(sp) \n "   // 保存 x31 寄存器的值
        "  sw x30, 24(sp) \n "   // 保存 x30 寄存器的值
        "  sw x29, 20(sp) \n "   // 保存 x29 寄存器的值
        "  sw x28, 16(sp) \n "   // 保存 x28 寄存器的值
        "  sw x27, 12(sp) \n "   // 保存 x27 寄存器的值
        "  mv x31, %0 \n "       // 将 x 的指针传递给 x31 寄存器
        "  mv x30, %1 \n "       // 将 y 的指针传递给 x30 寄存器
        "  mv x29, %2 \n "       // 将 n 的值传递给 x29 寄存器
        "  li x28, 0 \n "        // 将 i 初始化为 0
        "Loop_bit_flip_riscv: \n"
        "  beq x28, x29, Exit_bit_flip_riscv \n " // 如果 i == n，跳出循环
        "  lhu x27, 0(x31) \n "                   // 从 x[i] 加载半字到 x27，例如 x27 = 0x1234
        "  slli t0, x27, 8 \n "                   // 左移 8 位，t0 = 0x3400
        "  srli t1, x27, 8 \n "                   // 右移 8 位，提取高字节到 t1，t1 = 0x0012
        "  or t0, t0, t1 \n "                     // 将低字节和高字节重新组合，t0 = 0x3412
        "  sh t0, 0(x30) \n "                     // 将结果存储到 y[i]
        "  addi x31, x31, 2 \n "                  // x 指针增加 2
        "  addi x30, x30, 2 \n "                  // y 指针增加 2
        "  addi x28, x28, 1 \n "                  // i++
        "  j Loop_bit_flip_riscv \n"              // 跳转回循环开始
        "Exit_bit_flip_riscv: \n"
        "  lw x31, 28(sp) \n "  // 恢复 x31 寄存器的值
        "  lw x30, 24(sp) \n "  // 恢复 x30 寄存器的值
        "  lw x29, 20(sp) \n "  // 恢复 x29 寄存器的值
        "  lw x28, 16(sp) \n "  // 恢复 x28 寄存器的值
        "  lw x27, 12(sp) \n "  // 恢复 x27 寄存器的值
        "  addi sp, sp, 32 \n " // 恢复堆栈指针
        ::"r"(x),
        "r"(y), "r"(n)
        : "t0", "t1", "memory");
}

void bit_flip_riscv_pie(uint16_t *x, uint16_t *y, int n)
{
    asm volatile(
        "  addi sp, sp, -32 \n " // 为局部变量分配空间
        "  sw x31, 28(sp) \n "   // 保存 x31 寄存器的值
        "  sw x30, 24(sp) \n "   // 保存 x30 寄存器的值
        "  sw x29, 20(sp) \n "   // 保存 x29 寄存器的值
        "  sw x28, 16(sp) \n "   // 保存 x28 寄存器的值
        "  sw x27, 12(sp) \n "   // 保存 x27 寄存器的值
        "  mv x31, %0 \n "       // 将 x 的指针传递给 x31 寄存器
        "  mv x30, %1 \n "       // 将 y 的指针传递给 x30 寄存器
        "  mv x29, %2 \n "       // 将 n 的值传递给 x29 寄存器
        "  li x28, 0 \n "        // 将 i 初始化为 0
        "Loop_bit_flip_riscv_pie: \n"
        "  beq x28, x29, Exit_bit_flip_riscv_pie \n " // 如果 i == n，跳出循环
        "  lhu x27, 0(x31) \n "                   // 从 x[i] 加载半字到 x27，例如 x27 = 0x1234
        "  slli t0, x27, 8 \n "                   // 左移 8 位，t0 = 0x3400
        "  srli t1, x27, 8 \n "                   // 右移 8 位，提取高字节到 t1，t1 = 0x0012
        "  or t0, t0, t1 \n "                     // 将低字节和高字节重新组合，t0 = 0x3412
        "  sh t0, 0(x30) \n "                     // 将结果存储到 y[i]
        "  addi x31, x31, 2 \n "                  // x 指针增加 2
        "  addi x30, x30, 2 \n "                  // y 指针增加 2
        "  addi x28, x28, 1 \n "                  // i++
        "  j Loop_bit_flip_riscv_pie \n"              // 跳转回循环开始
        "Exit_bit_flip_riscv_pie: \n"
        "  lw x31, 28(sp) \n "  // 恢复 x31 寄存器的值
        "  lw x30, 24(sp) \n "  // 恢复 x30 寄存器的值
        "  lw x29, 20(sp) \n "  // 恢复 x29 寄存器的值
        "  lw x28, 16(sp) \n "  // 恢复 x28 寄存器的值
        "  lw x27, 12(sp) \n "  // 恢复 x27 寄存器的值
        "  addi sp, sp, 32 \n " // 恢复堆栈指针
        ::"r"(x),
        "r"(y), "r"(n)
        : "t0", "t1", "memory");
}