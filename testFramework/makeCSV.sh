#!/bin/bash
echo "Algorithm, Search strategy, Query, Modelname, Results, time(ms)" >> result.csv

for R in *.log; do
        awk '/Algorithm:/{print $NF}' $R >> meta.log
	awk '/Search:/{print $NF}' $R >> meta.log
	awk '/Query type:/{print $NF}' $R >> meta.log
	awk '/FORMULA /{print $3}' $R | tr -d '\n' >> meta.log
	echo "" >> meta.log
	awk '/Modelname:/{print $NF}' $R >> meta.log
	awk '/Total search elapsed time/{print $6}' $R >> is_tim.txt
	if [ -s is_tim.txt ]
		then sed -n -e '1, $ p' is_tim.txt >> meta.log
			 rm is_tim.txt
	else echo "did not finish" >> meta.log
		rm is_tim.txt
	fi


	for P in meta.log; do
	{
		read engine
                read search
		read q
		read formula
		read name
		read tim
		echo "$engine, $search, $q, $name, $formula, $tim" >> result.csv
	} < $P
	done
	rm meta.log
done
