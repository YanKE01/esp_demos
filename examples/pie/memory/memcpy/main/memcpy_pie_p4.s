    .data
    .align 16

    .text
    .align 4

    .global memcpy_pie
    .type   memcpy_pie, @function
memcpy_pie:
    # a0: store_ptr 
    # a1: load_ptr
    # a2: length(bytes)
    
    li x28,0
    Loop:
        esp.vld.128.ip q0, a1, 16
        esp.vld.128.ip q1, a1, 16
        esp.vld.128.ip q2, a1, 16
        esp.vld.128.ip q3, a1, 16
        esp.vld.128.ip q4, a1, 16
        esp.vld.128.ip q5, a1, 16
        esp.vld.128.ip q6, a1, 16
        esp.vld.128.ip q7, a1, 16

        esp.vst.128.ip q0, a0, 16
        esp.vst.128.ip q1, a0, 16
        esp.vst.128.ip q2, a0, 16
        esp.vst.128.ip q3, a0, 16
        esp.vst.128.ip q4, a0, 16
        esp.vst.128.ip q5, a0, 16
        esp.vst.128.ip q6, a0, 16
        esp.vst.128.ip q7, a0, 16

        addi x28, x28, 128
        bge x28, a2, exit
        j Loop
    exit:
        ret
