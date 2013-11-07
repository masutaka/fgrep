#!/bin/sh

echo "- 100MB -"

FGREPS="
fgrep
./c/fgrep
./ruby/fgrep.rb
"

for FGREP in $FGREPS; do
	echo $FGREP
	/usr/bin/time $FGREP defsubr 100MB.c > /dev/null
done

# echo
#
# echo "- 1GB -"
#
# /usr/bin/time ./c/fgrep defsubr 1GB.c > /dev/null
