#!/bin/sh

INPUT_FILE=$1
OUTPUT_FILE=1GB.c

while true; do
	cat ./100MB.c >> $OUTPUT_FILE
	OUTPUT_FILE_SIZE=`wc -c $OUTPUT_FILE | awk '{print $1}'`
	if [ "$OUTPUT_FILE_SIZE" -gt 1073741824 ]; then
		break
	fi
done
