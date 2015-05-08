#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#define NUM_GPIO        2
#define NUM_BITS        32
#define NUM_LED_ROWS    64
#define NUM_ROWS        (NUM_LED_ROWS + 2) // extra for start frame and end frame
#define NUM_STRIPS      32

#define BYTE_SIZE_SLICE (4 * 32 * NUM_GPIO * NUM_ROWS)

typedef enum {
    FX_RED,
    FX_GREEN,
    FX_BLUE,
    FX_WHITE,
    FX_STROBE
} Fx;

Fx effect = FX_STROBE;

uint32_t merge_led(uint8_t r, uint8_t g, uint8_t b) {
    return (0xff << 24) | (b << 16) | (g << 8) | r;
}

uint32_t fx_red() {
    return merge_led(255, 0, 0);
}

uint32_t fx_green() {
    return merge_led(255, 0, 0);
}

uint32_t fx_blue() {
    return merge_led(255, 0, 0);
}

uint32_t fx_white() {
    return merge_led(255, 255, 255);
}

uint32_t fx_strobe(int slice, int row, int col) {
    if (slice % 2 == 0) {
        return merge_led(0, 0, 0);
    } else {
        return merge_led(255, 255, 255);
    }
}

uint32_t* generateFrame(int sliceCount) {
    uint32_t* frameBuffer = calloc(1, BYTE_SIZE_SLICE * sliceCount);

    if (frameBuffer == NULL) {
        printf("Frame memory allocation failed.\n");
        exit(1);
    }

    for (int slice = 0; slice < sliceCount; slice++) {
        for (int row = 0; row < NUM_LED_ROWS; row++) {
            uint32_t ledRow[NUM_LED_ROWS]; // for led values in this row

            // generate pixels for this row
            for (int strip = 0; strip < NUM_STRIPS; strip++) {
                uint32_t value;

                switch (effect) {
                    case FX_RED:
                        value = fx_red();
                        break;
                    case FX_GREEN:
                        value = fx_green();
                        break;
                    case FX_BLUE:
                        value = fx_blue();
                        break;
                    case FX_WHITE:
                        value = fx_white();
                        break;
                    case FX_STROBE:
                        value = fx_strobe(slice, row, strip);
                        break;
                    default:
                        value = 0;
                }

                ledRow[strip] = value;
            }


            // peel off bits into dwords serialized for the 32 outputs

            uint32_t serialBuffer[NUM_BITS] = {0};

            for (int bit = 0; bit < NUM_BITS; bit++) {
                for (int strip = 0; strip < NUM_STRIPS; strip++) {
                    if (ledRow[strip] & (1 << bit))
                        serialBuffer[bit] |= 1 << strip;
                }
            }

            // split into the two GPIOs

            for (int idword = 0; idword < NUM_BITS; idword++) {
                uint32_t v = serialBuffer[idword];
                uint32_t GPIO1 = ((v & 0xff00) << 4) | (v & 0xff); // bits 0 -> 7 and 12 -> 19
                uint32_t GPIO2 = (v >> 16) << 2; // bits 2 -> 17

                // save to buffer (offset 1 for start frame)
                int index = NUM_GPIO * (NUM_BITS * (NUM_ROWS * slice + row + 1) + idword);

                frameBuffer[index  ] = GPIO1;
                frameBuffer[index+1] = GPIO2;
            }
        }

        // last row is all 1s
        for (int bit = 0; bit < 32; bit++) {
            int index = NUM_GPIO * (NUM_BITS * (NUM_ROWS * slice + NUM_ROWS-1) + bit);
            frameBuffer[index  ] = 0xff0ff;
            frameBuffer[index+1] = 0xffff << 2;
        }
    }

    return frameBuffer;
}

void print_usage(char* cmd) {
    printf("usage: %s #slices filename [--red | --green | --blue | --strobe]\n", cmd);
    exit(1);
}

int main(int argc, char* argv[]) {
    // parse cli arguments

    char* filename;
    int slices;

    if (argc == 4) {
        filename = argv[2];

        char* fx = argv[3];

        if (strcmp(fx, "--red") == 0) {
            effect = FX_RED;
        } else if(strcmp(fx, "--green") == 0) {
            effect = FX_GREEN;
        } else if(strcmp(fx, "--blue") == 0) {
            effect = FX_BLUE;
        } else if(strcmp(fx, "--white") == 0) {
            effect = FX_WHITE;
        } else if(strcmp(fx, "--strobe") == 0) {
            effect = FX_STROBE;
        } else {
            print_usage(argv[0]);
        }

        slices = atoi(argv[1]);

        if (slices == 0)
            printf("#slices must not be zero.\n");
    } else {
        print_usage(argv[0]);
    }

    // gen frame

    uint32_t* frame = generateFrame(slices);

    // save frame to file

    FILE *fd = fopen(filename, "w");
    fwrite(frame, 1, slices * BYTE_SIZE_SLICE, fd);

    return 0;
}

