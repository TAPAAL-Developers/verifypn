#!/bin/bash

inputfile=$1
outputfile=speedlog.log

echo "========================================================="
cat $inputfile | grep TIME_ELAPSE >> $outputfile
echo "========================================================="