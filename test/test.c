#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include <time.h>

#include "prussdrv.h"
#include <pruss_intc_mapping.h>

#include "pru-ctrl.h"

#define PRU_NUM 1

#define NUM_SLICES  8
#define NUM_GPIO    2
#define NUM_BITS    32
#define NUM_ROWS    (64 + 1) // extra for start frame
#define NUM_COLS    32

// # of unsplit dwords
#define NUM_DWORDS      (NUM_BITS * NUM_ROWS * NUM_SLICES)
#define NUM_FRAME_BYTES (NUM_DWORDS * NUM_GPIO * 4)

static int mem_fd;
static uint32_t* ddrMem;
static uint32_t* frameBuffer;

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

void initPRU() {
    int ret;

    // init pru driver
    prussdrv_init();

    // open PRU interrupt
    ret = prussdrv_open(PRU_EVTOUT_0);
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

    ret = pru_ctrl_init(mem_fd);

    if (ret == -1) {
        printf("Failed to initialize pru-ctrl (%s)\n", strerror(errno));
        close(mem_fd);
        exit(1);
    }

    // map the shared DDR memory
    uint32_t address = readHexFromFile("/sys/class/uio/uio0/maps/map1/addr");
    uint32_t size = readHexFromFile("/sys/class/uio/uio0/maps/map1/size");
    ddrMem = (uint32_t*)mmap(0, size, PROT_WRITE | PROT_READ, MAP_SHARED, mem_fd, address);

    if (ddrMem == NULL) {
        printf("Failed to map the device (%s)\n", strerror(errno));
        close(mem_fd);
        exit(1);
    }
}

void generateFrame() {
    srand(time(NULL));

    for (int slice = 0; slice < NUM_SLICES; slice++) {
        for (int row = 0; row < NUM_ROWS; row++) {
            uint32_t ledRow[NUM_ROWS]; // for led values in this row

            // generate pixels for this row
            if (row == 0) {
                // first row out is zeroes to init LEDs
                for (int col = 0; col < NUM_COLS; col++)
                    ledRow[col] = 0;
            } else {
                for (int col = 0; col < NUM_COLS; col++) {
                    uint8_t brightness = 0xff;
                    uint8_t r = rand();
                    uint8_t g = rand();
                    uint8_t b = rand();

                    ledRow[col] = brightness << 24 | b << 16 | g << 8 | r;
                }
            }

            // peel off bits into dwords serialized for the 32 outputs

            // # columns must be number of bits in each element
            uint32_t serialBuffer[NUM_BITS]; 

            for (int bit = 0; bit < NUM_BITS; bit++) {
                for (int col = 0; col < NUM_COLS; col++) {
                    serialBuffer[NUM_BITS - bit - 1] |= ((ledRow[col] >> bit) & 1) << col;
                }
            }

            for (int idword = 0; idword < NUM_BITS; idword++) {
                // split bits to match connections to GPIO registers
                uint32_t v = serialBuffer[idword];
                uint32_t GPIO0 = ((v & 0xff00) << 4) | (v & 0xff); // bits 0 -> 7 and 12 -> 19
                uint32_t GPIO2 = (v >> 16) << 2; // bits 2 -> 17

                // save to buffer
                int index = NUM_GPIO * (NUM_BITS * (NUM_ROWS * slice + row) + idword);

                frameBuffer[index  ] = GPIO0;
                frameBuffer[index+1] = GPIO2;
            }
        }
    }
}

int main(int argc, char* argv[]) {
    initPRU();
    frameBuffer = ddrMem;

    // clear PRU registers
    for (int i = 0; i < 30; i++)
        pru_write_reg(i, 0);

    uint32_t address = readHexFromFile("/sys/class/uio/uio0/maps/map1/addr");
    uint32_t size = readHexFromFile("/sys/class/uio/uio0/maps/map1/size");

    printf("ddr mem address is %x\n", address);
    printf("ddr mem size is %x\n", size);

    // give the PRU the address of shared DDR
    pru_write_reg(11, address);

    // single step the PRU
    pru_single_step();
    pru_load_program("./test-indexing.bin");

    int idword = 0;
    bool failed = false;

    while(idword < NUM_DWORDS && !failed) {
        while(pru_running());

        if (pru_read_pc() == 0x28) {
            // the split dword that would be output to the LEDs
            uint32_t readGPIO1 = pru_read_reg(1);
            uint32_t readGPIO2 = pru_read_reg(2);

            uint32_t expectedGPIO1 = frameBuffer[idword*2  ];
            uint32_t expectedGPIO2 = frameBuffer[idword*2+1];

            if (expectedGPIO1 != readGPIO1 || expectedGPIO2 != readGPIO2) {
                printf("failed! idword = %d\n", idword);
                printf("GPIO1 = %x, GPIO2 = %x\n", readGPIO1, readGPIO2);
                printf("expected %x and %x\n", expectedGPIO1, expectedGPIO2);

                failed = true;
            }

            idword += 1;
        }

        pru_enable();
    }

    prussdrv_pru_disable(PRU_NUM);

    prussdrv_exit();
    pru_ctrl_exit();

    // undo memory mapping
    munmap(ddrMem, 0x0FFFFFFF);
    close(mem_fd);

    printf("\ntests passed!\n");

    return(0);
}

