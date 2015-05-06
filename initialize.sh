#!/usr/bin/env sh
modprobe -r uio_pruss
modprobe uio_pruss extram_pool_sz=0x800000 # 8 megabytes
echo tube > /sys/devices/bone_capemgr.9/slots
