# FIXME no elegance here...

C_FLAGS += -Wall -O2 -mtune=cortex-a8 -march=armv7-a -std=gnu99

all:
	gcc -c $(C_FLAGS) -I../pru-ctrl ../pru-ctrl/pru-ctrl.c
	gcc -c $(C_FLAGS) -I../pru-ctrl -lprussdrv test.c
	gcc $(C_FLAGS) -o test test.o pru-ctrl.o -lprussdrv
	pasm -V2 -I../pru-ctrl -b test.p

.PHONY: all
