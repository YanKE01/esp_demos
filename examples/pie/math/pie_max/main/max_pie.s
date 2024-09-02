.data
.align 16

.text
.align 4

.global max_pie
max_pie:
    entry a1, 48
    movi a9, 0                  # 将a9初始化为0，作为循环计数器
pie_loop_start:
    ee.vld.128.ip q0, a2, 16    # 从a2 (x1)加载128位数据到q0
    ee.vld.128.ip q1, a2, 16    # 从a2 (x2)加载128位数据到q1
    ee.vld.128.ip q2, a2, 16    # 从a2 (x3)加载128位数据到q2
    ee.vld.128.ip q3, a2, 16    # 从a2 (x4)加载128位数据到q3
    ee.vld.128.ip q4, a3, 16    # 从a3 (y1)加载128位数据到q4
    ee.vld.128.ip q5, a3, 16    # 从a3 (y2)加载128位数据到q5
    ee.vld.128.ip q6, a3, 16    # 从a3 (y3)加载128位数据到q6
    ee.vld.128.ip q7, a3, 16    # 从a3 (y4)加载128位数据到q7

    ee.vmax.s16 q0, q0, q4     # 对q0和q4中的16位元素进行加法操作，结果存回q0
    ee.vmax.s16 q1, q1, q5     # 对q1和q5中的16位元素进行加法操作，结果存回q1
    ee.vmax.s16 q2, q2, q6     # 对q2和q6中的16位元素进行加法操作，结果存回q2
    ee.vmax.s16 q3, q3, q7     # 对q3和q7中的16位元素进行加法操作，结果存回q3

    ee.vst.128.ip q0, a4, 16    # 将q0中的结果存储回a4
    ee.vst.128.ip q1, a4, 16    # 将q1中的结果存储回a4
    ee.vst.128.ip q2, a4, 16    # 将q2中的结果存储回a4
    ee.vst.128.ip q3, a4, 16    # 将q3中的结果存储回a4

    addi a9, a9, 32              #每次读取了128*4位的数据，但我们每次都是16位的数据，所以一共包含128*4/16=32
    bge a9, a5, pie_loop_end    # 如果a9 >= a5，则跳转到pie_loop_end
    j pie_loop_start            # 跳转到pie_loop_start，继续下一次循环
pie_loop_end:
    retw                        # 返回指令，结束函数