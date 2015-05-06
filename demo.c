#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "pru-debug.h"
#include "pru-interface.hp"

#include "prussdrv.h"
#include <pruss_intc_mapping.h>

#define PRU_NUM 1

static int mem_fd;
static uint32_t* ddrMem;

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
    FILE* fp = fopen(filepath, "rb");

    // get the size

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // validate size
    
    if (fsize != NUM_FRAME_BYTES) {
        printf("Need file with %d bytes. Got file with %d bytes.\n", NUM_FRAME_BYTES, (unsigned int)fsize);
        exit(1);
    }

    if (get_ddr_size() < NUM_FRAME_BYTES) {
        printf("The size of shared memory is too small! Need %d bytes but only have %d.\n", NUM_FRAME_BYTES, get_ddr_size());
        printf("To set it correctly: \"modprobe uio_pruss extram_pool_sz=%x\"\n", NUM_FRAME_BYTES);
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
        filepath = "gen.dat";
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
    pru_debug_init(mem_fd);


    loadFrame(filepath);

    // tell the PRU where to find the data
    pru_write_reg(REG_ADDRESS, (uint32_t)get_ddr_address());

    // display!
    prussdrv_exec_program(PRU_NUM, "./driver.bin");
    getchar(); // hitting enter kills demo

    // cleanup

    // disable pru
    prussdrv_pru_disable(PRU_NUM);
    prussdrv_exit();
    pru_debug_exit();

    // undo memory mapping
    munmap(ddrMem, 0x0FFFFFFF);
    close(mem_fd);

    return 0;
}

