#include <pru.h>

#define RET_REG     r28.w0

#include <macros.p>

// offsets from CONST_IEP
#define IEP_REG_GLOBAL_CFG      0x00
#define IEP_REG_GLOBAL_STATUS   0x04
#define IEP_REG_COMPEN          0x08
#define IEP_REG_COUNT           0x0c
#define IEP_REG_CMP_CFG         0x40
#define IEP_REG_CMP_STATUS      0x44
#define IEP_REG_CMP0            0x48

#define PIN_ROTATION 13

#define rAddress r11

.setcallreg r28.w0

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

    // enable timer clock in the clock gating register
    lbco    r0, CONST_PRUCFG, 0x10, 4   
    set     r0, 17 // IEP clock enable bit
    sbco    r0, CONST_PRUCFG, 0x10, 4

    // set default & compensation inc to 1
    mov     r0, (1 << 8) | (1 << 4)
    sbco    r0, CONST_IEP, IEP_REG_GLOBAL_CFG, 4

    // disable compensation
    mov     r0, 0
    sbco    r0, CONST_IEP, IEP_REG_COMPEN, 4

    // disable all events
    mov     r0, 0
    sbco    r0, CONST_IEP, IEP_REG_CMP_CFG, 4

    // start IEP counter
    lbco    r0, CONST_IEP, IEP_REG_GLOBAL_CFG, 4
    set     r0, 0 // enable counter bit
    sbco    r0, CONST_IEP, IEP_REG_GLOBAL_CFG, 4

    // unsuspend the GPIO clocks
    StartClocks

reset_timer:
    // reset the count
    mov     r0, 0
    sbco    r0, CONST_IEP, IEP_REG_COUNT, 4
wait_for_sync:
    // wait for rotation sync
    qbbs    wait_for_sync, r31, PIN_ROTATION
wait_for_sync_end:
    qbbc    wait_for_sync_end, r31, PIN_ROTATION

    // tell C code how many milliseconds it took

    lbco    r0, CONST_IEP, IEP_REG_COUNT, 4
    sbbo    r0, rAddress, 0, 4

    jmp     reset_timer

die:
    // notify host program of finish
    mov     r31.b0, PRU0_ARM_INTERRUPT + 16
    halt
