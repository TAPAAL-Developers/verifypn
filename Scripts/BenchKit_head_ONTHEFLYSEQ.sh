#!/bin/bash

# This is the initialization script for the participation of TAPAAL
# untimed engine verifypn in the Petri net competition 2014.

# BK_EXAMINATION: it is a string that identifies your "examination"

# This is the 'on the fly sequential' tool.

export PATH="$PATH:/home/mcc/BenchKit/bin/"
VERIFYPN=$HOME/BenchKit/bin/verifypn
#VERIFYPN=/Users/srba/dev/sumoXMLparsing/verifypn-osx64
TIMEOUT=10

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
    echo "verifypn" $1 model.pnml $2  

    if [ $TIMEOUT = 0 ]; then
        $VERIFYPN $1 model.pnml $2
    else
        timeout $TIMEOUT $VERIFYPN $1 "model.pnml" $2
        RETVAL=$?
        if [ $RETVAL = 124 ] || [ $RETVAL =  125 ] || [ $RETVAL =  126 ] || [ $RETVAL =  127 ] || [ $RETVAL =  137 ] ; then
                echo -ne "CANNOT_COMPUTE\n"
        fi
    fi
} 


case "$BK_EXAMINATION" in
    StateSpace)
        echo		
        echo "*****************************************************"
        echo "*  TAPAAL ONTHEFLYSEQ performing StateSpace search  *"
        echo "*****************************************************"
        #$VERIFYPN -n -d -e model.pnml 
        echo "NOT IMPLEMENTED!"
        ;;

    ReachabilityComputeBounds)	
        echo		
        echo "*************************************************************"
        echo "*  TAPAAL ONTHEFLYSEQ performing ReachabilityComputeBounds  *"
        echo "*************************************************************"
        verify "-o" "ReachabilityComputeBounds.xml"
        ;;

    ReachabilityDeadlock)
        echo		
        echo "**********************************************************"
        echo "*  TAPAAL ONTHEFLYSEQ checking for ReachabilityDeadlock  *"
        echo "**********************************************************"
        TIMEOUT=0
        verify "-o" "ReachabilityDeadlock.xml"
        ;;

    ReachabilityCardinality)
        echo		
        echo "**********************************************************"
        echo "*  TAPAAL ONTHEFLYSEQ verifying ReachabilityCardinality  *"
        echo "**********************************************************"
        verify "-o" "ReachabilityCardinality.xml"
        ;;

    ReachabilityFireability)
        echo		
        echo "**********************************************************"
        echo "*  TAPAAL ONTHEFLYSEQ verifying ReachabilityFireability  *"
        echo "**********************************************************"
        verify "-o" "ReachabilityFireability.xml"
        ;;

    ReachabilityFireabilitySimple)
        echo
        echo "****************************************************************"
        echo "*  TAPAAL ONTHEFLYSEQ verifying ReachabilityFireabilitySimple  *"
        echo "****************************************************************"
        verify "-o" "ReachabilityFireabilitySimple.xml"
        ;;

    *)
        echo "DO_NOT_COMPETE"	
        exit 0
        ;;
esac
