#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include <math.h>

#include "prussdrv.h"
#include <pruss_intc_mapping.h>

#define PRU_NUM 1
#define PRU_INTGPR_REG  0x100
#define DDR_BASEADDR    0x80000000

static int mem_fd;
static uint32_t* ddrMem;
static uint32_t* pruMem;
static uint32_t* frameBuffer;

void initPRU() {
    // init pru driver
    prussdrv_init();

    // open PRU interrupt
    int ret = prussdrv_open(PRU_EVTOUT_0);
    if (ret) {
        printf("prussdrv_open open failed\n");
        exit(1);
    }

    // init interrupt
    tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;
    prussdrv_pruintc_init(&pruss_intc_initdata);

    // open memory device
    mem_fd = open("/dev/mem", O_RDWR);
    if (mem_fd < 0) {
        printf("Failed to open /dev/mem (%s)\n", strerror(errno));
        exit(1);
    }

    int pruss_len = 0x40000;
    int opt_pruss_addr = 0x4a300000;
    pruMem = (uint32_t*)mmap(0, pruss_len, PROT_WRITE | PROT_READ, MAP_SHARED, mem_fd, opt_pruss_addr);

    if (pruMem == NULL) {
        printf("Failed to map the device (%s)\n", strerror(errno));
        close(mem_fd);
        exit(1);
    }

    // map the memory
    ddrMem = (uint32_t*)mmap(0, 256 * 1024, PROT_WRITE | PROT_READ, MAP_SHARED, mem_fd, 0x9e5c0000);

    if (ddrMem == NULL) {
        printf("Failed to map the device (%s)\n", strerror(errno));
        close(mem_fd);
        exit(1);
    }
}

uint32_t readReg(int reg) {
    return pruMem[PRU_INTGPR_REG + 0x9000 + reg];
}

void writeReg(int reg, uint32_t value) {
    pruMem[PRU_INTGPR_REG + 0x9000 + reg] = value;
}

void generateFrame() {
    int sliceNum = 8;//100;
    int rowNum = 32;
    int colNum = 64;
    int bitNum = 32; // bits per LED
    int gpioNum = 2;

    if (frameBuffer != NULL)
        free(frameBuffer);

    //frameBuffer = (uint32_t*)malloc(4 * gpioNum * bitNum * colNum * sliceNum);
    frameBuffer = ddrMem;

    for (int slice = 0; slice < sliceNum; slice++) {
        double paletteIndex = slice * 2.56;

        for (int x = 0; x < colNum; x++) {
            uint32_t column[rowNum]; // led values in one column

            // generate pixels for this column
            for (int y = 0; y < rowNum; y++) {
                uint8_t brightness = 0xff;
                uint8_t r = cos(M_PI * paletteIndex / 128.0);
                uint8_t g = sin(M_PI * paletteIndex / 128.0);
                uint8_t b = 0;

                column[y] = brightness << 24 | b << 16 | g << 8 | r;
            }

            // peel off bits into dwords serialized for the 32 outputs

            uint32_t serialBuffer[bitNum];

            for (int bit = 0; bit < bitNum; bit++) {
                for (int idword = 0; idword < bitNum; idword++) {
                    serialBuffer[bit] |= ((column[idword] >> bit) & 1) << idword;
                }
            }

            for (int idword = 0; idword < bitNum; idword++) {
                // split bits to match connections to GPIO registers
                uint32_t v = serialBuffer[idword];
                uint32_t GPIO0 = ((v & 0xff00) << 4) | (v & 0xff); // bits 0 -> 7 and 12 -> 19
                uint32_t GPIO2 = (v >> 16) << 2; // bits 2 -> 17

                // save to buffer
                int index = gpioNum * (bitNum * (colNum * slice + x) + idword);

                frameBuffer[index  ] = 0xffffffff;//GPIO0;
                frameBuffer[index+1] = 0xeeeeeeee;//GPIO2;
            }
        }
    }
}

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

int main(int argc, char* argv[]) {
    initPRU();
    generateFrame();

    // give PRU the buffer address
    for (int i = 0; i < 30; i++)
        writeReg(i, 0);

    uint32_t address = readHexFromFile("/sys/class/uio/uio0/maps/map1/addr");
    uint32_t size = readHexFromFile("/sys/class/uio/uio0/maps/map1/size");

    printf("ddr mem address is %x\n", address);
    printf("ddr mem size is %x\n", size);
    writeReg(10, address);

    // check
    prussdrv_exec_program(PRU_NUM, "./driver.bin");
    sleep(1);
    prussdrv_pru_disable(PRU_NUM);
    
    printf("\n--- registers ---\n");
    for(int i = 0; i < 32; i++) {
        printf("r%d = %x\n", i, readReg(i));
    }

    printf("\n--- frame buffer ---\n");
    for(int i = 0; i < 8; i++) {
        printf("frameBuffer[%d] = %x\n", i, frameBuffer[i]);
    }

    // disable pru
    prussdrv_pru_disable(PRU_NUM);
    prussdrv_exit();

    // undo memory mapping
    munmap(ddrMem, 0x0FFFFFFF);
    close(mem_fd);

    printf("done\n");

    return(0);
}

