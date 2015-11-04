#!/bin/bash

errorlog=$1
speedlog=$2
resultlog=$3
correctlog=$4
outputfile="$5-result.log"

rm $outputfile

echo "=====================    Errors     =====================" >> $outputfile
cat $errorlog >> $outputfile
echo "===================== Speed Analysis=====================" >> $outputfile
cat $speedlog >> $outputfile
echo "=====================    Results    =====================" >> $outputfile
cat $resultlog >> $outputfile
echo "=====================  Correction   =====================" >> $outputfile
cat $correctlog >> $outputfile