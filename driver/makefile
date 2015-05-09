# FIXME no elegance here...

C_FLAGS += -Wall -O2 -mtune=cortex-a8 -march=armv7-a -std=gnu99

all:
	gcc -c $(C_FLAGS) -I../pru-ctrl ../pru-ctrl/pru-ctrl.c
	gcc -c $(C_FLAGS) -I../pru-ctrl -lprussdrv drive.c
	gcc $(C_FLAGS) -o drive drive.o pru-ctrl.o -lprussdrv
	pasm -V2 -I../pru-ctrl -b driver.p
	dtc -@ -O dtb -o tube-00A0.dtbo tube-00A0.dts

.PHONY: all
