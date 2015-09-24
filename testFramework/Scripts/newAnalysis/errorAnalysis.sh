#!/bin/bash

inputfile=$1
outputfile=errorlog.log

echo "========================================================="
cat $inputfile | grep ERROR >> $outputfile
echo "========================================================="