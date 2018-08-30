#! /bin/bash

[ -n "$1" ] || exit
frame=$1

list=$(./list.sh $1 | sort -n -k 3)
echo -n > "frame$frame.raw"
IFS=$'\n' 
for name in $list; do
#    echo "$name"
    cat "$name" >> "frame$frame.raw"
done
