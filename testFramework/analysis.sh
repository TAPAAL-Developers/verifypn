#!/bin/bash

inputfile=$1
outputfile=resultlog.log
csvoutputfile="result.csv"

runtime=$(cat $inputfile | grep real)
cat $inputfile | grep FORMULA | grep -v CANNOT_COMPUTE | grep -v DO_NOT_COMPETE >> $outputfile

cat $outputfile | awk '{print $2,$3}' FS=" " OFS=";" >> $csvoutputfile
cat $csvoutputfile | awk '{$(NF+1)=i++;}' FS=";" OFS=";" >> $csvoutputfile
rm $outputfile