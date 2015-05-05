C_FILES := test-indexing.c pru-debug.c
P_FILES := test-indexing.p

CC := gcc
LD := gcc
PASM := pasm

C_FLAGS += -Wall -O2 -mtune=cortex-a8 -march=armv7-a -std=gnu99
L_LIBS += -lprussdrv -lm

BIN_FILES := $(P_FILES:.p=.bin)
O_FILES := $(C_FILES:.c=.o)

all:	test $(BIN_FILES) $(DTBO_FILES)

test:	$(O_FILES)
	$(LD) -static -o $@ $(O_FILES) $(L_FLAGS) $(L_LIBS)

%.bin : %.p
	$(PASM) -V2 -I./ -b $<

%.o : %.c
	$(CC) $(C_FLAGS) -c -o $@ $<

.PHONY	: clean all
clean	:
	-rm -f $(O_FILES)
	-rm -f $(BIN_FILES)
