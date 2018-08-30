[ -n "$1" ] || exit
frame=$1

for name in frame*$frame/*; do
    [[ $name =~ invalid$ ]] && continue;
    [ -f "$name.invalid" ] && continue;
    [ -f "$name.partial" ] && continue;

    echo "$name"
done