#!/bin/bash

# This is the initialization script for the participation of TAPAAL
# untimed sequential single core engine verifypn in the Petri net 
# competition 2016 (MCC16).

# BK_EXAMINATION: it is a string that identifies your "examination"
export PATH="$PATH:/home/mcc/BenchKit/bin/"
VERIFYPN_SEQ=$HOME/BenchKit/bin/verifypn/verifypn-linux64
VERIFYPN_PAR=$HOME/BenchKit/bin/parallel-lockfree/verifypn-linux64
VERIFYPN_COM=$HOME/BenchKit/bin/SeqCTLCompression/verifypn-linux64

MEM="14500000"
ulimit -v $MEM

if [ ! -f iscolored ]; then
    	echo "File 'iscolored' not found!"
else
    if [ "TRUE" = $(cat "iscolored") ]; then
		echo "TAPAAL does not support colored nets."
		echo "DO_NOT_COMPETE" 
		exit 0
	fi
fi

if [ ! -f model.pnml ]; then
    	echo "File 'model.pnml' not found!"
	exit 1
fi

SEQ_LIST=" 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 "
PAR_LIST=""
COM_LIST=""
UNL_LIST=""

TIMEOUT_SEQ=75
TIMEOUT_PAR=300
TIMEOUT_COM=60

case "$BK_EXAMINATION" in
	CTLCardinality)
		echo		
		echo "**********************************************"
		echo "* TAPAAL Parallel verifying CTLCardinality *"
		echo "**********************************************" 

		for i in $SEQ_LIST ; do
			timeout $TIMEOUT_SEQ $VERIFYPN_SEQ -ctl czero -s DFS -n -x $i model.pnml CTLCardinality.xml
			RETVAL=$?
			if [ $RETVAL -ge 4 ] || [ $RETVAL -lt 0 ] || [ $RETVAL = 2 ]  ; then
				COM_LIST="$COM_LIST $i"
		    fi
		done

		for j in $COM_LIST ; do 
			timeout $TIMEOUT_COM $VERIFYPN_COM -ctl czero -s DFS -n -x $j model.pnml CTLCardinality.xml
			RETVAL=$?
			if [ $RETVAL -ge 4 ] || [ $RETVAL -lt 0 ] || [ $RETVAL = 2 ]  ; then
				PAR_LIST="$PAR_LIST $j"
		    fi
		done

		for k in $PAR_LIST ; do 
			timeout $TIMEOUT_PAR $VERIFYPN_PAR -ctl par -s DFS -n -x $k model.pnml CTLCardinality.xml
			RETVAL=$?
			if [ $RETVAL -ge 4 ] || [ $RETVAL -lt 0 ] || [ $RETVAL = 2 ]  ; then
				UNL_LIST="$UNL_LIST $k"
		    fi
		done
		for k in $UNL_LIST ; do 
			$VERIFYPN_PAR -ctl par -s DFS -n -x $k model.pnml CTLCardinality.xml
		done

		;;

	CTLFireability)
		echo		
		echo "**********************************************"
		echo "* TAPAAL Parallel verifying CTLFireability *"
		echo "**********************************************"

		for i in $SEQ_LIST ; do
			timeout $TIMEOUT_SEQ $VERIFYPN_SEQ -ctl czero -s DFS -n -x $i model.pnml CTLFireability.xml
			RETVAL=$?
			if [ $RETVAL -ge 4 ] || [ $RETVAL -lt 0 ] || [ $RETVAL = 2 ]  ; then
				COM_LIST="$COM_LIST $i"
		    fi
		done

		for j in $COM_LIST ; do 
			timeout $TIMEOUT_COM $VERIFYPN_COM -ctl czero -s DFS -n -x $j model.pnml CTLFireability.xml
			RETVAL=$?
			if [ $RETVAL -ge 4 ] || [ $RETVAL -lt 0 ] || [ $RETVAL = 2 ]  ; then
				PAR_LIST="$PAR_LIST $j"
		    fi
		done

		for k in $PAR_LIST ; do 
			timeout $TIMEOUT_PAR $VERIFYPN_PAR -ctl par -s DFS -n -x $k model.pnml CTLFireability.xml
			RETVAL=$?
			if [ $RETVAL -ge 4 ] || [ $RETVAL -lt 0 ] || [ $RETVAL = 2 ]  ; then
				UNL_LIST="$UNL_LIST $k"
		    fi
		done
		for k in $UNL_LIST ; do 
			$VERIFYPN_PAR -ctl par -s DFS -n -x $k model.pnml CTLFireability.xml
		done

		;;

	*)
		echo "DO_NOT_COMPETE"	
		exit 0
		;;
esac
