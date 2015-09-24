#!/bin/bash

inputfile=$1
outputfile=correctlog.log

echo "========================================================="
cat $inputfile | grep NOT_CORRECT >> $outputfile
echo "========================================================="