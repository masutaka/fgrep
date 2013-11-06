#!/bin/sh

INPUT_FILE=$1
OUTPUT_FILE=100MB.c

while true; do
	cat ./samples/alloc.c >> $OUTPUT_FILE
	OUTPUT_FILE_SIZE=`wc -c $OUTPUT_FILE | awk '{print $1}'`
	if [ "$OUTPUT_FILE_SIZE" -gt 104857600 ]; then
		break
	fi
done
