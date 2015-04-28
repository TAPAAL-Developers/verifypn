#!/bin/bash

# This is the initialization script for the participation of TAPAAL
# untimed engine verifypn in the Petri net competition 2014.

# BK_EXAMINATION: it is a string that identifies your "examination"

# This is the 'on the fly multicore' tool.

#export PATH="$PATH:/home/mcc/BenchKit/bin/"
#export PATH="$PATH:/home/mads/cpp/verifypnLTSmin/"
VERIFYPN=/home/mads/verifypnLTSmin/verifypn-linux64
TIMEOUT=25

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

    #if [ $TIMEOUT = 0 ]; then
    #    $VERIFYPN $1 model.pnml $2
    #else
    #    echo "timeout $TIMEOUT $VERIFYPN $1 model.pnml $2"
    #    timeout $TIMEOUT $VERIFYPN $1 "model.pnml" $2
    #    RETVAL=$?
    #    if [ $RETVAL = 124 ] || [ $RETVAL =  125 ] || [ $RETVAL =  126 ] || [ $RETVAL =  127 ] || [ $RETVAL =  137 ] ; then
    #            echo -ne "CANNOT_COMPUTE\n"
    #    fi
    #fi

        local NUMBER=`cat $2 | grep "<property>" | wc -l`
        for (( QUERY=1; QUERY<=$NUMBER; QUERY++ ))
        do
                echo
                echo "verifypn-linux64" $1 model.pnml $2  
                if [ $TIMEOUT = 0 ]; then
                        $VERIFYPN $1 "-x" $QUERY "model.pnml" $2
                else
                        timeout $TIMEOUT $VERIFYPN $1 "-x" $QUERY "model.pnml" $2
                        RETVAL=$?
                        if [ $RETVAL = 124 ] || [ $RETVAL =  125 ] || [ $RETVAL =  126 ] || [ $RETVAL =  127 ] || [ $RETVAL =  137 ] ; then
                                echo -ne "CANNOT_COMPUTE\n"
                        fi
                fi
        done  


} 


case "$BK_EXAMINATION" in
    StateSpace)
        echo		
        echo "*****************************************"
        echo "*  TAPAAL performing StateSpace search  *"
        echo "*****************************************"
        #verify "-o mc -r 1 -e"
        echo "NOT IMPLEMENTED!"
        ;;

    ReachabilityComputeBounds)	
        echo		
        echo "*************************************************"
        echo "*  TAPAAL performing ReachabilityComputeBounds  *"
        echo "*************************************************"
        verify "-o mc -r 1" "ReachabilityComputeBounds.xml"
        ;;

    ReachabilityDeadlock)
        echo		
        echo "**********************************************"
        echo "*  TAPAAL checking for ReachabilityDeadlock  *"
        echo "**********************************************"
        TIMEOUT=0
        verify "-o mc -r 1" "ReachabilityDeadlock.xml"
        ;;

    ReachabilityCardinality)
        echo		
        echo "**********************************************"
        echo "*  TAPAAL verifying ReachabilityCardinality  *"
        echo "**********************************************"
        verify "-o mc -r 1" "ReachabilityCardinality.xml"
        ;;

    ReachabilityFireability)
        echo		
        echo "**********************************************"
        echo "*  TAPAAL verifying ReachabilityFireability  *"
        echo "**********************************************"
        verify "-o mc -r 1" "ReachabilityFireability.xml"
        ;;

    ReachabilityFireabilitySimple)
        echo
        echo "****************************************************"
        echo "*  TAPAAL verifying ReachabilityFireabilitySimple  *"
        echo "****************************************************"
        verify "-o mc -r 1" "ReachabilityFireabilitySimple.xml"
        ;;

    *)
        echo "DO_NOT_COMPETE"	
        exit 0
        ;;
esac
