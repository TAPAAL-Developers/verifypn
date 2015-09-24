#!/bin/bash

inputfile=$1
outputfile=resultlog.log

echo "========================================================="
cat $inputfile | grep RESULT >> $outputfile
echo "========================================================="