what to expect
--------------

Solid colors or 8 slices alternating red and blue (former is --red, --green, --blue options of genframe and latter is --strobe option of the same).

running
-------

    sudo ./initialize.sh    # enable the PRU and load the device tree overlay (enable IO)
    ./genframe --strobe     # creates gen.dat to be read by demo program
    sudo ./demo             # just loads gen.dat into extMem and sets up the PRU

.dat files
----------

Contain raw data to be loaded into extMem and pushed out to the LEDs by the PRU.
It's a binary format where every pair of dwords corresponds to what will be written to the GPIO register pair needed to get data to the LEDs.
These masks indicate where the LED bits are in these 32 bit registers: 0x000ff0ff and 0003fffc
Generating an example file with genframe and looking at it in a hex viewer (like hexdump) might make the format clearer.
