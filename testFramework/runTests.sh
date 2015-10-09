#!/bin/bash
PROGRAMPATH=$1
MODELS=$2
ENGINE=$3
QUERYTYPE=$4
TIMEOUT=$5

cd 
ssh ikaufm12@frontend1.mcc.uppaal.org   
cd tools/test_1
#./run.sh $MODELS '$ENGINE' '$QUERYTYPE' $TIMEOUT

echo "Test started"