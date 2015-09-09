#!/bin/bash

# This is the initialization script for the participation of TAPAAL
# untimed engine verifypn in the Petri net competition 2014.

# BK_EXAMINATION: it is a string that identifies your "examination"

# This is the 'on the fly sequential' tool.

#export PATH="$PATH:/home/mcc/BenchKit/bin/"
#export PATH="$PATH:/home/sabella/Documents/verifypnLTSmin"
VERIFYPN=/home/mcc/BenchKit/bin/onthefly/verifypnLTSmin/verifypn-linux64
#VERIFYPN=/home/isabella/Documents/verifypnLTSmin/verifypn-linux64
#VERIFYPN=/Users/srba/dev/sumoXMLparsing/verifypn-osx64


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


        $VERIFYPN $1 "model.pnml" $2
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
        #$VERIFYPN -n -d -e model.pnml 
         $VERIFYPN -o seq -e -n -f seq model.pnml
        ;;

    ReachabilityComputeBounds)	
        echo		
        echo "*************************************************"
        echo "*  TAPAAL performing ReachabilityComputeBounds  *"
        echo "*************************************************"
        verify "-o seq -r 1 -f seq" "ReachabilityComputeBounds.xml"
        ;;

    ReachabilityBounds)  
        echo        
        echo "*************************************************"
        echo "*  TAPAAL performing ReachabilityBounds  *"
        echo "*************************************************"
        verify "-o seq -r 1 -f seq" "ReachabilityBounds.xml"
        ;;


    ReachabilityDeadlock)
        echo		
        echo "**********************************************"
        echo "*  TAPAAL checking for ReachabilityDeadlock  *"
        echo "**********************************************"
        $VERIFYPN -o seq -r 1 -f seq model.pnml ReachabilityDeadlock.xml
        ;;

    ReachabilityCardinality)
        echo		
        echo "**********************************************"
        echo "*  TAPAAL verifying ReachabilityCardinality  *"
        echo "**********************************************"
        verify "-o seq -r 1 -f seq" "ReachabilityCardinality.xml"
        ;;

    ReachabilityFireability)
        echo		
        echo "**********************************************"
        echo "*  TAPAAL verifying ReachabilityFireability  *"
        echo "**********************************************"
        verify "-o seq -r 1 -f seq" "ReachabilityFireability.xml"
        ;;

    ReachabilityFireabilitySimple)
        echo
        echo "****************************************************"
        echo "*  TAPAAL verifying ReachabilityFireabilitySimple  *"
        echo "****************************************************"
        verify "-o seq -r 1 -f seq" "ReachabilityFireabilitySimple.xml"
        ;;

    *)
        echo "DO_NOT_COMPETE"	
        exit 0
        ;;
esac
