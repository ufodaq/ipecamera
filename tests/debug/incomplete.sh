#! /bin/bash

function pci {
    PCILIB_PATH=`which pci`
    #LD_LIBRARY_PATH="$PCILIB_PATH" 
    $PCILIB_PATH/pci $*
}

rm -f images.raw
touch images.raw

echo "Starting the grabber"
pci --stop-dma
pci --start-dma dma1
pci -g -o images.raw --run-time 1000000000 &
pid=$!

usleep 1000000

for i in `seq 1 100000`; do
    old_size=`ls -la images.raw | cut -d " " -f 5`
    echo "Trigger $i"
    pci --trigger
    usleep 100000
    new_size=`ls -la images.raw | cut -d " " -f 5`
    if [ $old_size -eq $new_size ]; then
	sleep 2
	new_size=`ls -la images.raw | cut -d " " -f 5`
	if [ $old_size -eq $new_size ]; then
	    echo "Incomplete frame..."
	    killall -SIGINT pci
	    break
	fi
    fi
done

echo "Waiting grabber to finish"
wait $pid
