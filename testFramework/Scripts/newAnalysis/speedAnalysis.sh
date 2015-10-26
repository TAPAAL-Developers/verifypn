#!/bin/bash

inputfile=$1
outputfile=speedlog.log

echo "==================== Speed analysis ====================="
cat $inputfile | grep TIME_ELAPSE >> $outputfile