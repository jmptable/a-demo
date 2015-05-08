#ifndef H_PRU_CTRL
#define H_PRU_CTRL

#include <stdbool.h>
#include <stdint.h>

#define PRU_CTRL_RUNSTATE       15
#define PRU_CTRL_SINGLE_STEP    8
#define PRU_CTRL_COUNTER_ENABLE 3
#define PRU_CTRL_SLEEPING       2
#define PRU_CTRL_ENABLE         1
#define PRU_CTRL_SOFT_RST       0

// PRU1 region offsets
#define PRU_OFFSET_INSTRUCTIONS     0xE000
#define PRU_OFFSET_DATA             0x0800
#define PRU_OFFSET_CTRL             0x9000

// register offsets
#define PRU_CTRL_REG        0x0000
#define PRU_STATUS_REG      0x0001
#define PRU_INTGPR_REG      0x0100

int pru_ctrl_init(int mem_fd);
void pru_ctrl_exit();

void pru_load_program(char* filename);

void pru_enable();
void pru_freerun();
void pru_stop();

bool pru_running();
void pru_single_step();
void pru_step_into();

uint32_t pru_read_ctrl();
void pru_write_ctrl(uint32_t value);

uint32_t pru_read_pc();

uint32_t pru_read_reg(int reg);
void pru_write_reg(int reg, uint32_t value);

#endif
