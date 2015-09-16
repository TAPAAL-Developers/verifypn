#!/bin/bash


DATAFILE=$1
CSVFILENAME=$2

if [ "$1" == "-h" -o "$1" == "--help" ];
then
    echo
    echo "***************************** HELP ******************************"
    echo "*    Use: ./tool <data_file> <name_of_new_csv_file>             *"
    echo "*****************************************************************"
    echo
    exit
fi

if [ $# -eq 0 ];
then
    echo "Error: No arguments supplied"
    exit
fi

if [ -e "$CSVFILENAME" ];
then
    echo "Error: File \"$CSVFILENAME\" already exists"
    exit
else
    touch $CSVFILENAME
fi

#echo "Model name,ontheflyPAR-2,ontheflyPAR-4,ontheflyPAR-8,ontheflyPAR-16,classicMC-1" >> $CSVFILENAME

csv_output=""
skip_time="false"
print_result="false"

while IFS= read -r line
do
    if [[ "$line" == "/home/mossns/Documents/launchpad/INPUTS/"*  ]]
    then
        csv_output=$(echo $line | awk -F/ '{print $NF}')
    fi


    if [[ "$line" == "STATE_SPACE STATES "* ]]
    then
        #time_orig=$(echo $line | awk '{print $2}')
        #time_result=$(echo $time_orig | sed 's/s//' | awk -F"m" '{ print ($1 * 60) + $2 }')
	state_orig=$(echo $line | awk -F/ '{print $NF}')
        csv_output="$csv_output,$state_orig"
        print_result="true"
    fi

    if [[ "$line" == "STATE_SPACE MAX_TOKENS_IN_PLACE"* ]]
    then
        echo "$csv_output" >> "$CSVFILENAME"
    fi
done < "$DATAFILE"
