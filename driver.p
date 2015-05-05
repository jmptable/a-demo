#include <pru.h>

#define RET_REG     r28.w0

#include "macros.p"

#define RADDRESS r10

.setcallreg RET_REG

.origin 0

start:
    // enable OCP master port
    lbco    r0, CONST_PRUCFG, 4, 4
    clr     r0, r0, 4
    sbco    r0, CONST_PRUCFG, 4, 4

    // map ddr
    mov     r0, DDR_OFFSET << 8
    mov     r1, PRU1_CTRL + CTPPR1
    sbbo    r0, r1, 0, 4

    // unsuspend the GPIO clocks
    //StartClocks

main_loop:
    lbbo    r15, r10, 0, 4
    add     r10, r10, 4
    lbbo    r16, r10, 0, 4
    add     r10, r10, 4
    lbbo    r17, r10, 0, 4
    add     r10, r10, 4
    lbbo    r18, r10, 0, 4

    Delay   LONG_TIME

    jmp     main_loop

die:
    // notify host program of finish
    mov     r31.b0, PRU0_ARM_INTERRUPT + 16
    halt
