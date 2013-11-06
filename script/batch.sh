#!/bin/sh

FGREPS="
fgrep
./c/fgrep
./ruby/fgrep.rb
"

for FGREP in $FGREPS; do
	echo $FGREP
	/usr/bin/time $FGREP defsubr 100MB.c > /dev/null
done
