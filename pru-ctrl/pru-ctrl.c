#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "pru-ctrl.h"

#define PRU_LEN         0x40000
#define OPT_PRU_ADDR    0x4a300000

static uint32_t* pruMem;

int pru_ctrl_init(int mem_fd) {
    pruMem = (uint32_t*)mmap(0, PRU_LEN, PROT_WRITE | PROT_READ, MAP_SHARED, mem_fd, OPT_PRU_ADDR);

    return pruMem == NULL? -1 : 1;
}

void pru_ctrl_exit() {
    munmap(pruMem, PRU_LEN);
}

void pru_load_program(char* filepath) {
    // open file
    FILE* fp = fopen(filepath, "rb");

    // get size
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // read into PRU
    fread(pruMem + PRU_OFFSET_INSTRUCTIONS, fsize, 1, fp);

    fclose(fp);
}

void pru_enable() {
    uint32_t ctrl = pru_read_ctrl();
    pru_write_ctrl(ctrl | (1 << PRU_CTRL_ENABLE));
}

void pru_freerun() {
    uint32_t ctrl = pru_read_ctrl();

    ctrl |= 1 << PRU_CTRL_ENABLE;
    ctrl &= ~(1 << PRU_CTRL_SINGLE_STEP);

    pru_write_ctrl(ctrl);

    while (!(pru_read_ctrl() & (1 << PRU_CTRL_RUNSTATE)));
}

void pru_stop() {
    uint32_t ctrl = pru_read_ctrl();
    pru_write_ctrl(ctrl & ~(1 << PRU_CTRL_ENABLE));
    while (pru_read_ctrl() & (1 << PRU_CTRL_RUNSTATE));
}

bool pru_running() {
    return (pru_read_ctrl() & (1 << PRU_CTRL_RUNSTATE)) != 0;
}

void pru_single_step() {
    pru_write_ctrl(1 << PRU_CTRL_SINGLE_STEP);
}

void pru_step_into() {
    uint32_t ctrl = pru_read_ctrl();
    pru_write_ctrl(ctrl | (1 << PRU_CTRL_SINGLE_STEP) | (1 << PRU_CTRL_ENABLE));
}

uint32_t pru_read_ctrl() {
    return pruMem[PRU_OFFSET_CTRL + PRU_CTRL_REG];
}

void pru_write_ctrl(uint32_t value) {
    pruMem[PRU_OFFSET_CTRL + PRU_CTRL_REG] = value;
}

uint32_t pru_read_pc() {
    return pruMem[PRU_OFFSET_CTRL + PRU_STATUS_REG] & 0xFFFF;
}

uint32_t pru_read_reg(int reg) {
    return pruMem[PRU_OFFSET_CTRL + PRU_INTGPR_REG + reg];
}

void pru_write_reg(int reg, uint32_t value) {
    pruMem[PRU_OFFSET_CTRL + PRU_INTGPR_REG + reg] = value;
}
