#!/bin/bash

inputfile=$1
outputfile=resultlog.log

echo "==================== Result Analysis ===================="
cat $inputfile | grep FORMULA | grep -v CANNOT_COMPUTE | grep -v DO_NOT_COMPETE  >> $outputfile