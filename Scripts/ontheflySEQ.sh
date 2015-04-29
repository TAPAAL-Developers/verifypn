#!/bin/bash

# This is the initialization script for the participation of TAPAAL
# untimed engine verifypn in the Petri net competition 2014.

# BK_EXAMINATION: it is a string that identifies your "examination"

# This is the 'on the fly sequential' tool.

#export PATH="$PATH:/home/mcc/BenchKit/bin/"

#export PATH="$PATH:/home/mads/cpp/verifypnLTSmin/"

#VERIFYPN=/Users/srba/dev/sumoXMLparsing/verifypn-osx64
VERIFYPN=/home/mads/verifypnLTSmin/verifypn-linux64
#VERIFYPN=/Users/dyhr/Bazaar/verifypnLTSmin/verifypn-osx64

TIMEOUT=20

if [ ! -f iscolored ]; then
    echo "File 'iscolored' not found!"
else
    if [ "TRUE" = `cat iscolored` ]; then
        echo "TAPAAL ONTHEFLYSEQ does not support colored nets."
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

    if [ $TIMEOUT = 0 ]; then
        $VERIFYPN $1 model.pnml $2
    else
        gtimeout $TIMEOUT $VERIFYPN $1 "model.pnml" $2
        RETVAL=$?
        if [ $RETVAL = 124 ] || [ $RETVAL =  125 ] || [ $RETVAL =  126 ] || [ $RETVAL =  127 ] || [ $RETVAL =  137 ] ; then
                echo -ne "CANNOT_COMPUTE\n"
        fi
    fi
} 


case "$BK_EXAMINATION" in
    StateSpace)
        echo		
        echo "*****************************************"
        echo "*  TAPAAL performing StateSpace search  *"
        echo "*****************************************"

        gtimeout $TIMEOUT $VERIFYPN -o mc -d -e model.pnml 
        ;;

    ReachabilityComputeBounds)	
        echo		
        echo "*************************************************"
        echo "*  TAPAAL performing ReachabilityComputeBounds  *"
        echo "*************************************************"
        verify "-o seq" "ReachabilityComputeBounds.xml"
        ;;

    ReachabilityDeadlock)
        echo		
        echo "**********************************************"
        echo "*  TAPAAL checking for ReachabilityDeadlock  *"
        echo "**********************************************"
        TIMEOUT=0
        verify "-o seq" "ReachabilityDeadlock.xml"
        ;;

    ReachabilityCardinality)
        echo		
        echo "**********************************************"
        echo "*  TAPAAL verifying ReachabilityCardinality  *"
        echo "**********************************************"
        verify "-o seq" "ReachabilityCardinality.xml"
        ;;

    ReachabilityFireability)
        echo		
        echo "**********************************************"
        echo "*  TAPAAL verifying ReachabilityFireability  *"
        echo "**********************************************"
        verify "-o seq" "ReachabilityFireability.xml"
        ;;

    ReachabilityFireabilitySimple)
        echo
        echo "****************************************************"
        echo "*  TAPAAL verifying ReachabilityFireabilitySimple  *"
        echo "****************************************************"
        verify "-o seq" "ReachabilityFireabilitySimple.xml"
        ;;

    *)
        echo "DO_NOT_COMPETE"	
        exit 0
        ;;
esac
