#!/bin/bash

errorlog=$1
speedlog=$2
resultlog=$3
correctlog=$4

echo "======================== COMBINING ===================="
echo "================= Errors detected =========================" >> analysis.log
cat $errorlog >> analysis.log
echo "================= Speed Analysis =========================" >> analysis.log
cat $speedlog >> analysis.log
echo "================= Results =========================" >> analysis.log
cat $resultlog >> analysis.log
echo "================= Correctness =========================" >> analysis.log
cat $correctlog >> analysis.log
echo "======================================================="