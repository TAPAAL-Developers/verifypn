#!/bin/bash
csvoutputfile="$1-result.csv"

inputfile=$1
outputfile=resultlog.log

cat $inputfile | grep FORMULA | grep -v CANNOT_COMPUTE | grep -v DO_NOT_COMPETE  >> $outputfile

cat $outputfile | awk '{print $2,$3}' FS=" " OFS=";" >> $csvoutputfile