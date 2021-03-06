#include <pru.h>

#define RET_REG     r28.w0

#include <macros.p>

#define PIN_ROTATION    13
#define PIN_CLOCK       22

#define NUM_SLICES      8
#define NUM_ROWS        32
#define NUM_BITS        32
#define NUM_COLS        64

#define MASK_CLOCK      1 << PIN_CLOCK
#define MASK_GPIO1      0x000ff0ff
#define MASK_GPIO2      (0xffff << 2)

#define rClear1         r4
#define rSet1           r5
#define rClear2         r6
#define rSet2           r7

#define rMaskClock      r8
#define rMask1          r9
#define rMask2          r10

#define rAddress        r11
#define rCurrentAddress r12
#define rSliceCount     r13
#define rColCount       r14
#define rBitCount       r15

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

    // prepare IO addresses
    mov     rClear1, GPIO1 | GPIO_CLEARDATAOUT
    mov     rSet1, GPIO1 | GPIO_SETDATAOUT
    mov     rClear2, GPIO2 | GPIO_CLEARDATAOUT
    mov     rSet2, GPIO2 | GPIO_SETDATAOUT

    // prepare masks
    mov     rMaskClock, MASK_CLOCK
    mov     rMask1, MASK_GPIO1
    mov     rMask2, MASK_GPIO2

    // unsuspend the GPIO clocks
    StartClocks

rotation_sync:
    // wait for rotation sync
    qbbs    rotation_sync, r31, PIN_ROTATION

    // reset the address
    mov     rCurrentAddress, rAddress

    ldi     rSliceCount, NUM_SLICES
slice_start:
    ldi     rColCount, NUM_COLS + 1 // extra for start frame
col_start:
    ldi     rBitCount, NUM_BITS
col_out:
    // load the next column of bits
    lbbo    r1, rCurrentAddress, 0, 4
    lbbo    r2, rCurrentAddress, 4, 4
    add     rCurrentAddress, rCurrentAddress, 8

    // done LED column?
    dec     rBitCount
    qbne    col_out, rBitCount, 0

    // done slice?
    dec     rColCount
    qbne    col_start, rColCount, 0

    // done frame?
    dec     rSliceCount
    qbne    slice_start, rSliceCount, 0

    jmp     rotation_sync

die:
    // notify host program of finish
    mov     r31.b0, PRU0_ARM_INTERRUPT + 16
    halt
