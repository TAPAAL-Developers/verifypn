#!/bin/bash

errorlog=$1
speedlog=$2
resultlog=$3
correctlog=$4


echo "======================= Combining ======================="
echo "=====================    Errors     =====================" >> analysis.log
cat $errorlog >> analysis.log
echo "===================== Speed Analysis=====================" >> analysis.log
cat $speedlog >> analysis.log
echo "=====================    Results    =====================" >> analysis.log
cat $resultlog >> analysis.log
echo "=====================  Correction   =====================" >> analysis.log
cat $correctlog >> analysis.log