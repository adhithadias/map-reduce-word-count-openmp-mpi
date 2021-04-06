#!/bin/bash

NEWFILE=$1

for file in `ls|sort -g -r`
do
    filename=$(basename "$file")
    extension=${filename##*.}
    filename=${filename%.*}

    # if [ $filename -ge $NEWFILE ]
    if [ true ]
    then
        echo "copying file $file"
        cp "$file" "$(($filename + 120))".$extension
    fi
done