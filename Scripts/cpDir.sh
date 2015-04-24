#!/bin/bash
# This script copies the smallest instances of the models

INPUTDIR=$1
MODELNAME=""
CURDIRNAME=""

if [ "$1" == "-h" ];
then
    echo "***************************** HELP ******************************"
    echo "*    The models are located in a directory.                     *
*    Rename this directory to something different from INPUTS.  * 
*    Call this tool with the new directory name.                *"
    echo "*****************************************************************"
    exit
fi

if [ $# -eq 0 ];
then
    echo "No arguments supplied"
    exit
fi

if [ -d "INPUTS" ];
then
    echo 'Output dir "INPUTS" already exists'
    exit
else
    mkdir INPUTS
fi

for dir in $(find $INPUTDIR -type d | sort -V); do 

    if [[ "$dir" != *"-PT-"* ]]; 
    then 
        continue
    fi

    CURDIRNAME=$(echo $dir | cut -d'/' -f 2 | cut -d'-' -f 1)

    if [[ $CURDIRNAME != $MODELNAME ]];
    then
        MODELNAME=$CURDIRNAME
        #echo $dir
        cp -r $dir INPUTS
    fi
done
