.data
.align 16

.text
.align	4

.global add
add:
    entry a1, 48
    movi a9, 0

loop_start:
    # 加载x[i]到a10
    l16ui a10, a2, 0        # 从x指针(a2)加载16位数据到a10
    # 加载y[i]到a11
    l16ui a11, a3, 0        # 从y指针(a3)加载16位数据到a11

    # 执行加法运算
    add a12, a10, a11       # a12 = a10 + a11

    # 存储结果到z[i]
    s16i a12, a4, 0         # 将a12存储到z指针(a4)

    # 更新指针位置和循环计数器
    addi a2, a2, 2          # x指针(a2)向前移动2字节
    addi a3, a3, 2          # y指针(a3)向前移动2字节
    addi a4, a4, 2          # z指针(a4)向前移动2字节
    addi a9, a9, 1          # 增加循环索引a9

    # 检查是否达到循环终点
    blt a9, a5, loop_start  # 如果a9 < n (a5)，则跳回loop_start

    # 清理并返回
    retw                    # 返回指令，结束函数