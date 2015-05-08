This tool is for checking the integrity of LED data files
and for displaying basic information about them (# slices).

Currently checks for these problems:

* file size not a multiple of the size of a slice
* illegal bits set in the GPIO registers (corresponding to pins that are not connected to the leds)

Running the tool with no arguments prints usage information.
