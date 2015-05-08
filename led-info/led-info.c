#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#define NUM_LED_ROWS    64
#define NUM_ROWS        (NUM_LED_ROWS + 2)
#define NUM_STRIPS      32
#define NUM_GPIO        2

#define BYTE_SIZE_SLICE (4 * 32 * NUM_GPIO * NUM_ROWS)

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

void validate_frame(uint32_t* frame, int len) {
    int problemCount = 0;

    for (int i = 0; i < len; i += 2) {
        uint32_t gpio1 = frame[i];
        uint32_t gpio2 = frame[i+1];

        uint32_t masked1 = gpio1 & ~0xff0ff;
        uint32_t masked2 = gpio2 & ~(0xffff << 2);

        if (masked1 != 0) {
            printf("frame[%d] has illegal GPIO1 bits set (%x)\n", i, masked1);
            problemCount++;
        }

        if (masked2 != 0) {
            printf("frame[%d] has illegal GPIO2 bits set (%x)\n", i, masked2);
            problemCount++;
        }

        if (problemCount > 16) {
            printf("More problems than I will display...\n");
            exit(1);
        }
    }

    if (problemCount == 0)
        printf("Frame data looks good!\n");
}

void print_led_info(char* filepath) {
    if (access(filepath, F_OK) == -1) {
        printf("file %s does not exist.", filepath);
        exit(1);
    }

    FILE* fp = fopen(filepath, "rb");

    // get the size

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // validate size
    
    if (fsize % BYTE_SIZE_SLICE != 0) {
        printf("Problem:\tData length is not a multiple of the slice size (%d bytes).\n", BYTE_SIZE_SLICE);
        exit(1);
    } else {
        printf("Info:\tContains %d slices.\n", (int)fsize / BYTE_SIZE_SLICE);
    }

    if (get_ddr_size() < fsize) {
        printf("Warning: The current size of shared memory is too small! Need %d bytes for this frame but only have %d.\n", (int)fsize, get_ddr_size());
        exit(1);
    }

    // read file into shared memory

    uint32_t* frame = (uint32_t*)malloc(fsize);
    fread(frame, fsize, 1, fp);
    fclose(fp);

    validate_frame(frame, fsize / 4);
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

    print_led_info(filepath);

    return 0;
}

