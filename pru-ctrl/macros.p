#define LONG_TIME   0xf00000

#define PRU0_CTRL   0x22000
#define PRU1_CTRL   0x24000

#define CTPPR0      0x28
#define CTPPR1      0x2C 

#define OWN_RAM     0x000
#define OTHER_RAM   0x020
#define SHARED_RAM  0x100
#define DDR_OFFSET  0

#define ADDR_SHARED_RAM 0x80000000
#define ADDR_DDR_RAM    0xc0000000

#define GPIO0_CLOCK 0x44e00408
#define GPIO1_CLOCK 0x44e000ac
#define GPIO2_CLOCK 0x44e000b0

.macro inc
.mparam reg
    add     reg, reg, 1
.endm

.macro dec
.mparam reg
    sub     reg, reg, 1
.endm

.macro Delay
.mparam len
    mov     r0, len
delay_loop:
    sub     r0, r0, 1
    qbne    delay_loop, r0, 0
.endm

// value to set pin on gpio high into r0
.macro IOHigh
.mparam gpio, bit
    mov     r0, gpio | GPIO_SETDATAOUT
    mov     r1, 1 << bit
    sbbo    r1, r0, 0, 4
.endm

// value to set pin on gpio low into r0
.macro IOLow
.mparam gpio, bit
    mov     r0, gpio | GPIO_CLEARDATAOUT
    mov     r1, 1 << bit
    sbbo    r1, r0, 0, 4
.endm

.macro StartClocks
    mov     r0, 1 << 1 // set bit 1 in reg to enable clock
    mov     r1, GPIO0_CLOCK
    sbbo    r0, r1, 0, 4
    mov     r1, GPIO1_CLOCK
    sbbo    r0, r1, 0, 4
    mov     r1, GPIO2_CLOCK
    sbbo    r0, r1, 0, 4
.endm

.macro Die
    // notify host program of finish
    mov     r31.b0, PRU0_ARM_INTERRUPT + 16
    halt
.endm
