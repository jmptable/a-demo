#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#define NUM_SLICES  8
#define NUM_GPIO    2
#define NUM_BITS    32
#define NUM_ROWS    (64 + 1) // extra for start frame
#define NUM_COLS    32

// # of unsplit dwords
#define NUM_DWORDS      (NUM_BITS * NUM_ROWS * NUM_SLICES)
#define NUM_FRAME_BYTES (NUM_DWORDS * NUM_GPIO * 4)

uint32_t* generateFrame() {
    uint32_t* frameBuffer = malloc(NUM_FRAME_BYTES);

    if (frameBuffer == NULL) {
        printf("Frame memory allocation failed.\n");
        exit(1);
    }

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
                    uint8_t r = 0xff;
                    uint8_t g = 0xff;
                    uint8_t b = 0xff;

                    ledRow[col] = brightness << 24 | b << 16 | g << 8 | r;
                }
            }

            // peel off bits into dwords serialized for the 32 outputs

            // # columns must be number of bits in each element
            uint32_t serialBuffer[NUM_BITS] = {0};

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

    return frameBuffer;
}

int main(int argc, char* argv[]) {
    // parse cli arguments

    char* filename;

    if (argc == 2) {
        filename = argv[1];
    } else {
        filename = "gen.dat";
    }

    // gen frame

    uint32_t* frame = generateFrame();

    // save frame to file

    FILE *fd = fopen(filename, "wb");
    fwrite(frame, NUM_FRAME_BYTES, 1, fd);

    return 0;
}

