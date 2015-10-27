#!/bin/bash
IFS=","

while read f1 f2 f3 f4 f5 f6 f7
do
     if [ $f4 != DNF ]; then
     	colorstring=COL
     	if [ "${f2/$colorstring}" = "$f2" ]; then
	     	echo "Model: $f2 Examination: $f3 Result: $f5 Estimated Result: $f7"
	     	echo "Query 0: ${f5:0:1}"
	     	echo "Query 1: ${f5:1:1}"
	     	echo "Query 2: ${f5:2:1}"
	     	echo "Query 3: ${f5:3:1}"
	     	echo "Query 4: ${f5:4:1}"
	     	echo "Query 5: ${f5:5:1}"
	     	echo "Query 6: ${f5:6:1}"
	     	echo "Query 7: ${f5:7:1}"
	     	echo "Query 8: ${f5:8:1}"
	     	echo "Query 9: ${f5:9:1}"
	     	echo "Query 10: ${f5:10:1}"
	     	echo "Query 11: ${f5:11:1}"
	     	echo "Query 12: ${f5:12:1}"
	     	echo "Query 13: ${f5:13:1}"
	     	echo "Query 14: ${f5:14:1}"
	     	echo "Query 15: ${f5:15:1}"
	     	echo ""
     	fi
     fi
done < MCCMarcieRes.csv