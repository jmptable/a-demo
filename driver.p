#include <pru.h>

#define RET_REG     r28.w0

#include "macros.p"

#define PIN_ROTATION 13

#define NUM_SLICES  8
#define NUM_ROWS    32
#define NUM_BITS    32
#define NUM_COLS    64

#define rDataAddress    r10
#define rSliceCount     r11
#define rColCount       r12
#define rBitCount       r13

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
    StartClocks

interrupt_loop:
    // wait for rotation sync
    qbbs    interrupt_loop, r31, PIN_ROTATION
start_frame:
    lbbo    r15, r10, 0, 4
    add     r15, r15, 1
    sbbo    r15, r10, 0, 4

temp: // wait for sync to end
    qbbc    temp, r31, PIN_ROTATION

    jmp     interrupt_loop

die:
    // notify host program of finish
    mov     r31.b0, PRU0_ARM_INTERRUPT + 16
    halt

write_data:
    mov     r9, 32
write_data_next:
    qbbs    write_data_1, r10, 0
write_data_0:
    mov     r0, GPIO1 | GPIO_CLEARDATAOUT
    mov     r1, 0xff0ff
    sbbo    r1, r0, 0, 4

    mov     r0, GPIO2 | GPIO_CLEARDATAOUT
    mov     r1, 0x3fffc
    sbbo    r1, r0, 0, 4

    qba     write_data_end
write_data_1:
    mov     r0, GPIO1 | GPIO_SETDATAOUT
    mov     r1, 0xff0ff
    sbbo    r1, r0, 0, 4

    mov     r0, GPIO2 | GPIO_SETDATAOUT
    mov     r1, 0x3fffc
    sbbo    r1, r0, 0, 4
write_data_end:
    IOLow   GPIO2, 22 // clock out
    Delay   0xf000

    IOHigh  GPIO2, 22
    Delay   0xf000

    lsr     r10, r10, 1

    dec     r9
    qbne    write_data_next, r9, 0

    ret
