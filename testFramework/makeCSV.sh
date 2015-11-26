#!/bin/bash
echo "Search strategy, Query, Modelname, Results, time(sec), mem(kb)" >> result.csv

for R in *.log; do
	awk '/Search:/{print $NF}' $R >> meta.log
	awk '/Query type:/{print $NF}' $R >> meta.log
	awk '/FORMULA/{print $3}' $R | tr -d '\n' >> meta.log
	echo "" >> meta.log
	awk '/Modelname:/{print $NF}' $R >> meta.log
	awk '/real/{print $NF}' $R >> is_tim.txt
	if [ -s is_tim.txt ]
		then sed -n -e '1, $ p' is_tim.txt >> meta.log
			 rm is_tim.txt
	else echo "did not finish" >> meta.log
		rm is_tim.txt
	fi

	awk '/comsumption:/{print $NF}' $R >> is_mem.txt

	if [ -s is_mem.txt ]
		then sed -n -e '1, $ p' is_mem.txt >> meta.log
			 rm is_mem.txt
	else echo "could not detect" >> meta.log
		rm is_mem.txt
	fi

	for P in meta.log; do
	{
		read engine
		read q
		read formula
		read name
		read tim
		read mem
		echo "$engine, $q, $name, $formula, $tim, $mem" >> result.csv
	} < $P
	done
	rm meta.log
done
