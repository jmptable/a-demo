C_FILES := demo.c pru-debug.c
P_FILES := driver.p
DTS_FILES := tube-00A0.dts

CC := gcc
LD := gcc
PASM := pasm
DTC := dtc

C_FLAGS += -Wall -O2 -mtune=cortex-a8 -march=armv7-a -std=gnu99
L_LIBS += -lprussdrv -lm

BIN_FILES := $(P_FILES:.p=.bin)
O_FILES := $(C_FILES:.c=.o)
DTBO_FILES := $(DTS_FILES:.dts=.dtbo)

all:	demo $(BIN_FILES) $(DTBO_FILES)

demo:	$(O_FILES)
	$(LD) -static -o $@ $(O_FILES) $(L_FLAGS) $(L_LIBS)

%.bin : %.p
	$(PASM) -V2 -I./ -b $<

%.o : %.c
	$(CC) $(C_FLAGS) -c -o $@ $<

%.dtbo : %.dts
	$(DTC) -@ -O dtb -o $@ $<

.PHONY	: clean all
clean	:
	-rm -f $(O_FILES)
	-rm -f $(BIN_FILES)
	-rm -f $(DTBO_FILES)
	-rm -f demo
