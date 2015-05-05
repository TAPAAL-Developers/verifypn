#!/bin/bash

# This is the initialization script for the participation of TAPAAL
# untimed engine verifypn in the Petri net competition 2014.

# BK_EXAMINATION: it is a string that identifies your "examination"

# This is the 'on the fly multicore' tool.

#export PATH="$PATH:/home/mcc/BenchKit/bin/"
#export PATH="$PATH:/home/mads/cpp/verifypnLTSmin/"
#export PATH="$PATH:/home/sabella/Documents/verifypnLTSmin"
#VERIFYPN=/home/isabella/Documents/verifypnLTSmin/verifypn-linux64
#VERIFYPN=/home/mcc/BenchKit/bin/onthefly/verifypnLTSmin/verifypn-linux64
VERIFYPN=/Users/dyhr/Bazaar/verifypnLTSmin/verifypn-osx64

if [ ! -f iscolored ]; then
    echo "File 'iscolored' not found!"
else
    if [ "TRUE" = `cat iscolored` ]; then
        echo "TAPAAL ONTHEFLYMC does not support colored nets."
        echo "DO_NOT_COMPETE" 
        exit 0
    fi
fi

if [ ! -f model.pnml ]; then
    echo "File 'model.pnml' not found!"
    exit 1
fi

function verify {
    if [ ! -f $2 ]; then
        echo "File '$2' not found!" 
        exit 1 
    fi
    echo
    echo "verifypn-linux64" $1 model.pnml $2  

    $VERIFYPN $1 "model.pnml" $2
} 


case "$BK_EXAMINATION" in
    StateSpace)
        echo "#EXAMINATION StateSpace search"
        echo		
        gtimeout 10 $VERIFYPN -o mc -e -c $CORES model.pnml
        ;;

    ReachabilityComputeBounds)	
        echo		
        echo "*************************************************"
        echo "*  TAPAAL performing ReachabilityComputeBounds  *"
        echo "*************************************************"
        verify "-o mc -r 1 -f par" "ReachabilityComputeBounds.xml"
        ;;

    ReachabilityBounds)  
         echo        
        echo "*************************************************"
        echo "*  TAPAAL performing ReachabilityBounds  *"
        echo "*************************************************"
        verify "-o seq -r 1 -f par" "ReachabilityBounds.xml"
        ;;

    ReachabilityDeadlock)
        echo		
        echo "**********************************************"
        echo "*  TAPAAL checking for ReachabilityDeadlock  *"
        echo "**********************************************"
        verify "-o mc -r 1 -f par" "ReachabilityDeadlock.xml"
        ;;

    ReachabilityCardinality)
        echo		
        echo "**********************************************"
        echo "*  TAPAAL verifying ReachabilityCardinality  *"
        echo "**********************************************"
        verify "-o mc -f par" "ReachabilityCardinality.xml"
        ;;

    ReachabilityFireability)
        echo		
        echo "**********************************************"
        echo "*  TAPAAL verifying ReachabilityFireability  *"
        echo "**********************************************"
        verify "-o mc -r 1 -f par" "ReachabilityFireability.xml"
        ;;

    ReachabilityFireabilitySimple)
        echo
        echo "****************************************************"
        echo "*  TAPAAL verifying ReachabilityFireabilitySimple  *"
        echo "****************************************************"
        verify "-o mc -r 1 -f par" "ReachabilityFireabilitySimple.xml"
        ;;

    *)
        echo "DO_NOT_COMPETE"	
        exit 0
        ;;
esac
