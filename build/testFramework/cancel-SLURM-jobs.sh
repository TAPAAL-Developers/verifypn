#!/bin/bash
count=$1
MAX=$2
while [ $count -le $MAX ] ;
do
	squeue | grep "smni12" | grep $count
	if [ "$?" = "0" ]; then 
		scancel $count
		echo Canceled
	fi
	let "count += 1" 
done 

