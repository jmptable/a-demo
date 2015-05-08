#!/usr/bin/env sh

die () {
    echo >&2 "$@"
    exit 1
}

if [ "$(id -u)" != "0" ]; then
    die "This script must be run as root so that the hardware can be accessed."
fi

# validate parameters
[ "$#" -eq 1 ] || die "usage: $0 some-led-data.dat"

# initialize hardware if necessary
if [ ! -f /sys/class/uio/uio0/maps/map1/addr ]; then
    echo "Initializing hardware..."

    modprobe -r uio_pruss
    modprobe uio_pruss extram_pool_sz=0x800000 # 8 megabytes
    echo tube > /sys/devices/bone_capemgr.9/slots

    if [ ! -f /sys/class/uio/uio0/maps/map1/addr ]; then
        die "The hardware failed to initialize for some reason. Take a look at dmesg maybe?"
    fi
fi

driver/drive $1
