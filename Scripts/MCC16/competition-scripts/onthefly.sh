#!/bin/bash

TIMEOUT=10
VERIFYPN=$HOME/BenchKit/bin/verifypnLTSmin

MEM="14500000"
ulimit -v $MEM

if [ ! -f iscolored ]; then
    	echo "File 'iscolored' not found!"
else
	if [ "TRUE" = `cat iscolored` ]; then
		echo "TAPAAL does not support colored nets."
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
		echo "verifypn" $1 "model.pnml" $2
		if [ $TIMEOUT = 0 ]; then
			$VERIFYPN $1 "model.pnml" $2
		else
			for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 ; do
				timeout $TIMEOUT $VERIFYPN/verifypn-linux64 $1 "model.pnml" $2 -x $i
			done
			$VERIFYPN/verifypn-linux64 $1 -d "model.pnml" $2 
			RETVAL=$?
			if [ $RETVAL = 124 ] || [ $RETVAL =  125 ] || [ $RETVAL =  126 ] || [ $RETVAL =  127 ] || [ $RETVAL =  137 ] ; then
				echo "the return value is $RETVAL"
				echo -ne "CANNOT_COMPUTE\n"
				
			fi
		fi
} 


echo
echo "*****************************************"
echo "  TAPAAL " onthefly-PAR " performing $BK_EXAMINATION search"
echo "*****************************************"

case "$BK_EXAMINATION" in

	StateSpace)
		$VERIFYPN/verifypn-linux64 -o mc -c 4 -n -d -e model.pnml 
		;;

	UpperBounds)	
		verify "-o mc -c 4 -n" "UpperBounds.xml"
		;;

	ReachabilityDeadlock)
		$VERIFYPN/verifypn-linux64 -o mc -c 4 -n -d model.pnml ReachabilityDeadlock.xml
		;;

	ReachabilityCardinality)
		verify "-o mc -c 4 -n" "ReachabilityCardinality.xml"
		;;

	ReachabilityFireability)
		verify "-o mc -c 4 -n" "ReachabilityFireability.xml"
		;;

	*)
		echo "DO_NOT_COMPETE"	
		exit 0
		;;
esac
