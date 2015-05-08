This is the main tube driver code.

The `drive` command loads a specified LED data file into memory that the PRU is able to access, then sets up the PRU and cleans up after it when it's done.

It must be run as root (`sudo ./driver`) so that it can have a proper chance at fubarring memory and hardware.
