.data
.align 16

b_mask:
    .short 0x1F00
r_mask:
    .short 0x00F8
gh_mask:
    .short 0xE000
gl_mask:
    .short 0x0007
gfh_mask:
    .short 0x0038 # 最后去g的高位
ones:
    .short 1
eight:
    .short 8
tfs:
    .short 256 # two five six 就是左移8 = *256
eont:
    .short 8192 # 8192 就是左移 13位
.text
.align 4

.literal b_mask_a, b_mask
.literal r_mask_a, r_mask
.literal gh_mask_a, gh_mask
.literal gl_mask_a, gl_mask
.literal ones_a, ones
.literal eight_a, eight
.literal tfs_a, tfs
.literal eont_a, eont
.literal gfh_mask_a, gfh_mask

.global rgb565_channel_process
rgb565_channel_process:
    entry a1, 48
    # a2 buffer
    # a3 output
    # a4 n
    # a5 scale

    movi a8, 8 # 乘法移位控制
    ssr a8
    wsr.sar	a8

    movi a9, 0 # 总体数量计数
    l32r a12, ones_a

process_channel:
    ee.vld.128.ip q0, a2, 16
    ee.vldbc.16 q1, a12 # one mask

# 处理 B 通道
    movi a8, 8
    ssr a8
    l32r a13, b_mask_a
    ee.vldbc.16 q2, a13 # b mask
    ee.andq q3, q0, q2
    ee.vmul.u16 q3, q3, q1 #q3存放的是B

# 处理 R 通道
    movi a8, 3
    ssr a8
    l32r a13, r_mask_a
    ee.vldbc.16 q2, a13 # r mask
    ee.andq q4, q0, q2
    ee.vmul.u16 q4, q4, q1 #  q4存放的是R

# 处理 G 通道 gl<<3|gh
    movi a8, 13
    ssr a8
    l32r a13,gh_mask_a
    ee.vldbc.16 q2,a13 # gh mask
    ee.andq q5, q0, q2
    ee.vmul.u16 q5, q5, q1

    l32r a13, gl_mask_a
    ee.vldbc.16 q2,a13 # gl mask
    ee.andq q6, q0, q2

    # gl mask <<3 | gh_mask，相当于乘以8 + gh_mask
    movi a8, 0
    ssr a8
    l32r a13,eight_a
    ee.vldbc.16 q2,a13
    ee.vmul.u16 q6, q6, q2
    ee.vadds.s16 q5,q5,q6   # q5 存放的是G

    # 可以用的有Q1 Q2 Q6 Q7
# 处理缩放
    movi a8, 8
    ssr a8
    ee.vldbc.16 q1, a5

    ee.vmul.u16 q3,q3,q1 # b
    ee.vmul.u16 q4,q4,q1 # r
    ee.vmul.u16 q5,q5,q1 # g
# 拼接
    movi a8, 0
    ssr a8

    # r << 3
    l32r a13,eight_a
    ee.vldbc.16 q2,a13
    ee.vmul.u16 q4,q4,q2 # r

    # b << 8
    l32r a13,tfs_a
    ee.vldbc.16 q2,a13
    ee.vmul.u16 q3,q3,q2 # b

    # g & 0x07 且 << 13
    l32r a13,gl_mask_a
    ee.vldbc.16 q2,a13
    ee.andq q6,q5,q2
    l32r a13,eont_a
    ee.vldbc.16 q2,a13
    ee.vmul.u16 q6,q6,q2

    # g& 0x38 >> 3
    l32r a13,gfh_mask_a
    ee.vldbc.16 q2,a13
    ee.andq q7,q5,q2
    movi a8, 3
    ssr a8
    l32r a13, ones_a
    ee.vldbc.16 q1, a13
    ee.vmul.u16 q7,q7,q1

# 拼接结果
    ee.vadds.s16 q7,q7,q6 # 拼完G
    ee.vadds.s16 q7,q7,q3 # 评完b
    ee.vadds.s16 q7,q7,q4 # 评完r
    ee.vst.128.ip q7,a3,16

# 整体循环技术
    addi a9, a9, 8
    bge a9, a4, pie_loop_end
    j process_channel
pie_loop_end:
    retw
