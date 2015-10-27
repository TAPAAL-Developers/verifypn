#!/bin/bash

inputfile=$1
outputfile=resultlog.log
csvoutputfile=resultlog.csv

echo "==================== Result Analysis ===================="
cat $inputfile | grep FORMULA | grep -v CANNOT_COMPUTE | grep -v DO_NOT_COMPETE  >> $outputfile

rm $csvoutputfile

cat $outputfile | awk '{print $2,$3}' FS=" " OFS=";" >> $csvoutputfile