#! /bin/bash

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
pci -g -o /dev/null --run-time 12000000 --verbose 10 &
pid=$!

sleep 0.1
pci -w 9040 80004a01
sleep 10
pci -w 9040 80000201

echo "Waiting grabber to finish"
wait $pid
