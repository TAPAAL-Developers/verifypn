#!/bin/bash

# This is the initialization script for the participation of TAPAAL
# untimed engine verifypn in the Petri net competition 2015.
# It uses a number of available cores.

# BK_EXAMINATION: it is a string that identifies your "examination"

export PATH="$PATH:/home/isabella/Documents/ontheflyMC"
VERIFYPN=/home/isabella/Documents/verifypnLTSmin/verifypn-linux64
TIMEOUT=500

if [ ! -f iscolored ]; then
    	echo "File 'iscolored' not found!"
else
	if [ "TRUE" = `cat iscolored` ]; then
		echo "TAPAAL ONTHEFLYPAR does not support colored nets."
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
	local NUMBER=`cat $2 | grep "<property>" | wc -l`

        seq 1 $NUMBER | 
	parallel --will-cite -j4 -- "timeout $TIMEOUT $VERIFYPN $1 "-x" {} "-pid" {} "model.pnml" $2 ; RETVAL=\$? ;\
		if [ \$RETVAL = 124 ] || [ \$RETVAL =  125 ] || [ \$RETVAL =  126 ] || [ \$RETVAL =  127 ] || [ \$RETVAL =  137 ] ; then echo -ne \"CANNOT_COMPUTE\n\"; fi"
} 


case "$BK_EXAMINATION" in

	StateSpace)
		echo		
		echo "*****************************************"
		echo "*  TAPAAL performing StateSpace search  *"
		echo "*****************************************"
               timeout $TIMEOUT $VERIFYPN -o seq -e -n model.pnml
		;;

	ReachabilityComputeBounds)	
		echo		
		echo "***********************************************"
		echo "*  TAPAAL verifying ReachabilityComputeBounds *"
		echo "***********************************************"
                verify "-o seq -r 1" "ReachabilityComputeBounds.xml"
		;;

	ReachabilityDeadlock)
		echo		
		echo "**********************************************"
		echo "*  TAPAAL checking for ReachabilityDeadlock  *"
		echo "**********************************************"
		TIMEOUT=10
                verify "-o seq -r 1" "ReachabilityDeadlock.xml"
		;;

	ReachabilityCardinality)
		echo		
		echo "**********************************************"
		echo "*  TAPAAL verifying ReachabilityCardinality  *"
		echo "**********************************************"
		verify "-o seq " "ReachabilityCardinality.xml"
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
                echo "****************************************************************"
                echo "*  TAPAAL ONTHEFLYPAR verifying ReachabilityFireabilitySimple  *"
                echo "****************************************************************"
                verify "-o seq" "ReachabilityFireabilitySimple.xml"
                ;;

	*)
		echo "DO_NOT_COMPETE"	
		exit 0
		;;
esac