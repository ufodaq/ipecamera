#! /bin/bash

function pci {
    PCILIB_PATH=`which pci`
    #LD_LIBRARY_PATH="$PCILIB_PATH" 
    $PCILIB_PATH/pci $*
}

echo "Starting the grabber"
pci -g -o /dev/null --run-time 12000000 --verbose 10 &
pid=$!

usleep 100000
pci -w 9040 80004a01
usleep 10000000
pci -w 9040 80000201

echo "Waiting grabber to finish"
wait $pid
