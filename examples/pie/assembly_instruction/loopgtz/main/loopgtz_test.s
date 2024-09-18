    .align 4
    .text
    .global loopgtz_test
    .type       loopgtz_test, @function
loopgtz_test:
    .align 4
    entry sp, 32           
    l32i a4, a2, 0             
    loopgtz a3, end          
    addi a4, a4, 1
end:
    s32i a4, a2, 0  
    retw
