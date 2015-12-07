#!/bin/bash
echo "Algorithm, Search strategy, Query, Modelname, #Results)" >> result.csv

for R in *.log; do
        awk '/Algorithm:/{print $NF}' $R >> meta.log
	awk '/Search:/{print $NF}' $R >> meta.log
	awk '/Query type:/{print $NF}' $R >> meta.log
	awk '/FORMULA /{print $3}' $R | tr -d '\n' >> meta.log
	echo "" >> meta.log
	awk '/Modelname:/{print $NF}' $R >> meta.log




	for P in meta.log; do
	{
		read engine
                read search
		read q
		read formula
		read name
		echo "$engine, $search, $q, $name, $formula" >> result.csv
	} < $P
	done
	rm meta.log
done
