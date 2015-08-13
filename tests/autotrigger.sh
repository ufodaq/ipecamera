#! /bin/bash

#Example options:
# ./autotrigger.sh --threads 4 							- use multiple threads for preprocessing
# ./autotrigger.sh --data raw --format raw					- test raw_data_callback functionality
# PCILIB_DEBUG_MISSING_EVENTS=1 IPECAMERA_DEBUG_HARDWARE=1 ./autotrigger.sh	- debug missing frames 

function pci {
    APP_PATH=`dirname $0`/..
    if [ -d $APP_PATH/../pcitool ]; then
        PCILIB_BINARY="$APP_PATH/../pcitool/pcitool/pci"
        PCILIB_PATH="$APP_PATH/../pcitool/pcilib"
    else
	PCILIB_BINARY=`which pci`
	PCILIB_PATH=""
    fi
    
    LD_LIBRARY_PATH="$PCILIB_PATH" PCILIB_PLUGIN_DIR="$APP_PATH" $PCILIB_BINARY $*
}

echo "Starting the grabber"
pci -g -o /dev/null --run-time 10002000000 --verbose 10 $@ &
pid=$!

trap "{ kill -s INT $!; }" SIGINT

sleep 0.1
pci -w 9040 80004a01
sleep 10000
pci -w 9040 80000201

echo "Waiting grabber to finish"
wait $pid
