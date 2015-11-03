#!/bin/bash

inputfile=$1
outputfile=speedlog.log

cat $inputfile | grep TIME_ELAPSE >> $outputfile