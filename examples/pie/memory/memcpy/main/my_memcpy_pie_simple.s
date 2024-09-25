.data
.align 16

.text
.align 4

.global my_memcpy_pie_simple
my_memcpy_pie_simple:
    entry a1, 32
    # a2: store_ptr
    # a3: load_ptr
    # a4: length(bytes)
    movi a9, 0

pie_loop_start:
    ee.vld.128.ip q0, a3, 16
    ee.vld.128.ip q1, a3, 16
    ee.vld.128.ip q2, a3, 16
    ee.vld.128.ip q3, a3, 16
    ee.vld.128.ip q4, a3, 16

    ee.vst.128.ip q0, a2, 16
    ee.vst.128.ip q1, a2, 16
    ee.vst.128.ip q2, a2, 16
    ee.vst.128.ip q3, a2, 16
    ee.vst.128.ip q4, a2, 16

    addi a9, a9, 80
    bge a9, a4, pie_loop_end
    j pie_loop_start

pie_loop_end:
    retw