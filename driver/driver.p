#include <pru.h>

#define RET_REG     r29.w0

#include <macros.p>
#include "pru-interface.hp"

// offsets from CONST_IEP
#define IEP_REG_GLOBAL_CFG      0x00
#define IEP_REG_GLOBAL_STATUS   0x04
#define IEP_REG_COMPEN          0x08
#define IEP_REG_COUNT           0x0c
#define IEP_REG_CMP_CFG         0x40
#define IEP_REG_CMP_STATUS      0x44
#define IEP_REG_CMP0            0x48

#define PIN_ROTATION    13
#define PIN_CLOCK       8

#define MASK_CLOCK      1 << PIN_CLOCK
#define MASK_GPIO1      0x000ff0ff
#define MASK_GPIO2      (0xffff << 2)

#define rClear1         r2
#define rSet1           r3
#define rClear2         r4
#define rSet2           r5

#define rMask1          r6
#define rMask2          r7

#define rCurrentAddress r8
#define rSliceCount     r9
#define rRowCount       r10

#define rTime           r11

// separates register into the right GPIO bitfields,
// sets the GPIOs, and clocks out the data to the LEDs
.macro SplitAndClockOut
.mparam reg
    // clear old output
    sbbo    rMask1, rClear1, 0, 4
    sbbo    rMask2, rClear2, 0, 4

    // fill the GPIO bitfields with the value
    mov     r0.b0, reg.b1
    lsl     r0, r0, 12
    mov     r0.b0, reg.b0
    lsl     r1.w0, reg.w1, 2

    // set new output
    sbbo    r0, rSet1, 0, 4
    sbbo    r1, rSet2, 0, 4

    // clock out bit column
    clr     r30, PIN_CLOCK
    set     r30, PIN_CLOCK
.endm

.macro HalfRowOut
    SplitAndClockOut    r14
    SplitAndClockOut    r15
    SplitAndClockOut    r16
    SplitAndClockOut    r17
    SplitAndClockOut    r18
    SplitAndClockOut    r19
    SplitAndClockOut    r20
    SplitAndClockOut    r21

    SplitAndClockOut    r22
    SplitAndClockOut    r23
    SplitAndClockOut    r24
    SplitAndClockOut    r25
    SplitAndClockOut    r26
    SplitAndClockOut    r27
    SplitAndClockOut    r28
    SplitAndClockOut    r29
.endm

.macro SetupIEPCounter
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
.endm

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
    mov     rMask1, MASK_GPIO1
    mov     rMask2, MASK_GPIO2

    // unsuspend the GPIO clocks
    StartClocks

rotation_sync:
    // wait for rotation sync
    qbbs    rotation_sync, r31, PIN_ROTATION
wait_for_sync_end: // TODO deal with underlying issue
    qbbc    wait_for_sync_end, r31, PIN_ROTATION

    // reset the pointer to the current data
    mov     rCurrentAddress, rAddress

    // rSliceTotal was set by the host application
    mov     rSliceCount, rSliceTotal
slice_start:
    ldi     rRowCount, NUM_ROWS
row_start:
    // indirection so the quick branches can reach back up
    jmp     row_out
row_out_done:
    // done slice?
    dec     rRowCount
    qbne    row_start, rRowCount, 0

    // done frame?
    dec     rSliceCount
    qbne    slice_start, rSliceCount, 0

    jmp     rotation_sync

row_out:
    // load the data for first half of the row
    lbbo    r14, rCurrentAddress, 0, 32
    lbbo    r22, rCurrentAddress, 32, 32

    // send it to the LEDs
    HalfRowOut

    // load the data for second half of the row
    lbbo    r14, rCurrentAddress, 64, 32
    lbbo    r22, rCurrentAddress, 96, 32

    // send it to the LEDs
    HalfRowOut

    // increment our pointer to the next row
    add     rCurrentAddress, rCurrentAddress, 128 

    jmp     row_out_done
