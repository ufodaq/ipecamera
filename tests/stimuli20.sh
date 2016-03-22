#!/bin/bash
# By Michele Caselle for UFO 6 / 20 MPixels - camera
echo "-----------------------------"
echo "----  S T I M U L I ---------"
echo "-----------------------------"

if [ -z "$1" ]; then
    echo "Please specify number of frames required"
    exit
fi

rm mult$2.out*
rd_flag=1

#Make wr higher priority than rd
pci -w 0x9100 0x20001000
ddr_thr=a0

echo "Set Number of frames.. to $1 hex"
pci -w 9170 $1

sleep .1
pci -w 9040 0x80000001
sleep .1
echo "Send mult frame request ... "
pci -w 9040 0x80000211
sleep .05

#pci -g -s $1 -o mult$2.raw 
#IPECAMERA_DEBUG_HARDWARE="1" pci -g -s $1 --run-time 10000000 --buffer 1024 --verbose -o mult$2.out.decoded
IPECAMERA_DEBUG_HARDWARE="1" pci -g -s $1 --run-time 10000000 --format default --data raw --buffer 1024 --verbose -o mult$2.out 
sleep .3
echo "decoding..."
sleep .1
ipedec -r 3840 --num-columns 5120 mult$2.out -f --continue

sleep .1
pci -w 9040 0x80000201
pci -r 9000 -s 100
