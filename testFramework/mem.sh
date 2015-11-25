#!/bin/bash
while true
do 
	clear
	awk '/Rss/ {print "+", $2}' /proc/`pidof verifypn-linux64`/smaps | bc >> txt.txt
	INPUT=$(awk '$0>x{x=$0};END{print x}' txt.txt)
	if [ $INPUT = ""]; then 
		rm txt.txt
		break;
		else if [ $INPUT -eq $INPUT 2> /dev/null ]; then 
		awk '$0>x{x=$0};END{print x}' txt.txt >> mem.txt
		rm txt.txt
		fi
	fi
done 
Result=$(awk '$0>x{x=$0};END{print x}' mem.txt)
rm mem.txt
echo "final memory comsumption: $Result"