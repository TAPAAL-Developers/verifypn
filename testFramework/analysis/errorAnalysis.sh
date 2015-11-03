#!/bin/bash

inputfile=$1
outputfile=errorlog.log


echo "==================== Compute errors ====================" >> $outputfile
cat $1 | grep FORMULA | grep CANNOT_COMPUTE >> $outputfile
echo "==================== Consistency errors ====================" >> $outputfile
cat $1 | grep FORMULA | grep -v CANNOT | awk '{print $1,$2,$3}' | sort | uniq | awk '{print $2}' | uniq -d >> $outputfile