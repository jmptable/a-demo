#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "pru-ctrl.h"
#include "pru-interface.hp"

#include "prussdrv.h"
#include <pruss_intc_mapping.h>

#define PRU_NUM 1

// TODO fewer globals...

static int mem_fd;
static uint32_t* ddrMem;
int sliceCount;

uint32_t readHexFromFile(char* filepath) {
    FILE* fp = fopen(filepath, "r");

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* buffer = malloc(fsize + 1);
    fread(buffer, fsize, 1, fp);
    fclose(fp);

    uint32_t value;
    sscanf(buffer, "%x", &value);
    free(buffer);

    return value;
}

uint32_t* get_ddr_address() {
    static uint32_t* address = NULL;

    if (address == NULL)
        address = (uint32_t*)readHexFromFile("/sys/class/uio/uio0/maps/map1/addr");

    return address;
}

uint32_t get_ddr_size() {
    static uint32_t size = ~0;

    if (size == ~0)
        size = readHexFromFile("/sys/class/uio/uio0/maps/map1/size");

    return size;
}

void init_memory() {
    // open memory device
    mem_fd = open("/dev/mem", O_RDWR);
    if (mem_fd < 0) {
        printf("Failed to open /dev/mem (%s)\n", strerror(errno));
        exit(1);
    }

    // map the shared DDR memory
    uint32_t size = get_ddr_size();
    uint32_t* address = get_ddr_address();
    ddrMem = (uint32_t*)mmap(0, size, PROT_WRITE | PROT_READ, MAP_SHARED, mem_fd, (int)address);

    if (ddrMem == NULL) {
        printf("Failed to map the device (%s)\n", strerror(errno));
        close(mem_fd);
        exit(1);
    }
}

void loadFrame(char* filepath) {
    if (access(filepath, F_OK) == -1) {
        printf("%s not found!", filepath);
        exit(1);
    }

    FILE* fp = fopen(filepath, "rb");

    // get the size

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // validate size
    
    if (fsize % BYTE_SIZE_SLICE != 0) {
        printf("LED data needs to be divisible by the size of a slice (%d bytes).\n", BYTE_SIZE_SLICE);
        exit(1);
    }

    sliceCount = fsize / BYTE_SIZE_SLICE;

    if (get_ddr_size() < fsize) {
        printf("The size of shared memory is too small! Need %d bytes but only have %d.\n", (int)fsize, get_ddr_size());
        exit(1);
    }

    // read file into shared memory
    fread(ddrMem, fsize, 1, fp);
    fclose(fp);
}

int main(int argc, char* argv[]) {
    // parse cli arguments

    char* filepath;

    if (argc == 2) {
        filepath = argv[1];
    } else {
        printf("usage: %s some-led-data.dat\n", argv[0]);
        exit(1);
    }

    prussdrv_init();

    int ret = prussdrv_open(PRU_EVTOUT_0);
    if (ret) {
        printf("prussdrv_open open failed\n");
        exit(1);
    }

    tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;
    prussdrv_pruintc_init(&pruss_intc_initdata);

    init_memory();

    // initialize interface to PRU
    pru_ctrl_init(mem_fd);

    loadFrame(filepath);

    // tell the PRU where to find the data
    pru_write_reg(REG_ADDRESS, (uint32_t)get_ddr_address());

    // tell the PRU how many slices we have
    pru_write_reg(REG_SLICE_TOTAL, sliceCount);

    // display!
    prussdrv_exec_program(PRU_NUM, "./driver.bin");
    printf("running...\nhit enter to stop.\n");
    getchar();

    // cleanup

    // disable pru
    prussdrv_pru_disable(PRU_NUM);
    prussdrv_exit();
    pru_ctrl_exit();

    // undo memory mapping
    munmap(ddrMem, 0x0FFFFFFF);
    close(mem_fd);

    return 0;
}

