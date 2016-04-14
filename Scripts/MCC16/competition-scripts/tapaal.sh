#!/bin/bash

# This is the initialization script for the participation of TAPAAL
# untimed sequential single core engine verifypn in the Petri net 
# competition 2016 (MCC16).

# BK_EXAMINATION: it is a string that identifies your "examination"

export PATH="$PATH:/home/mcc/BenchKit/bin/"
VERIFYPN=$HOME/BenchKit/bin/verifypn-linux64

#Allowed memory in kB
MEM="14500000"
ulimit -v $MEM

#STATISTICS="/usr/bin/time -f \"###%e,%M###\""
#STATISTICS="/usr/local/bin/gtime -f \"###%e,%M###\""

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


function verify {
        if [[ $2 == "" ]] ; then
            echo "No more queries"
            exit 1
        fi
        RQ=""
        QS=$(echo $2)
	for QUERY in $QS ; do
  		echo
                echo "timeout" $1 "verifypn" $3 "-x" "$QUERY" "model.pnml" $4
                       timeout $1 $VERIFYPN $3 "-x" $QUERY model.pnml $4
                RETVAL=$?
    #            if [ $RETVAL = 124 ] || [ $RETVAL =  125 ] || [ $RETVAL =  126 ] || \
    #               [ $RETVAL =  127 ] || [ $RETVAL =  137 ] || [ $RETVAL = 134 ] ; then
		if [ $RETVAL -ge 4 ] || [ $RETVAL -lt 0 ] || [ $RETVAL = 2 ]  ; then
			RQ="$RQ $QUERY"
                fi	 

        done
        LIST="$RQ"
} 

function getlist
{
    NUMBER=`cat $1 | grep "<property>" | wc -l`

    QS=""
    for (( QUERY=1; QUERY<=$NUMBER; QUERY++ )) ;
    do
        QS="$QS $QUERY"
    done
    echo $QS
}

case "$BK_EXAMINATION" in

	StateSpace)
		echo		
		echo "**************************************************"
		echo "* TAPAAL Sequential performing StateSpace search *"
		echo "**************************************************"
		$VERIFYPN -n -d -e model.pnml 
                exit 0 
		;;

	UpperBounds)	
		echo		
		echo "*******************************************"
		echo "* TAPAAL Sequential verifying UpperBounds *"
		echo "*******************************************"
                LIST=$(getlist "UpperBounds.xml")
                verify 7200 "$LIST" "-n -d -r 1 -s BFS" "UpperBounds.xml"
                exit 0 
		;;

	ReachabilityDeadlock)
		echo		
		echo "*******************************************************"
		echo "* TAPAAL Sequential checking for ReachabilityDeadlock *"
		echo "*******************************************************"
                LIST=$(getlist "ReachabilityDeadlock.xml")
                verify 60 "$LIST" "-n -d -r 1 -s BFS" "ReachabilityDeadlock.xml"
                verify 60 "$LIST" "-n -d -r 1" "ReachabilityDeadlock.xml"
                verify 7200 "$LIST" "-n -d -r 1 -s DFS" "ReachabilityDeadlock.xml"
                exit 0 
		;;

	ReachabilityCardinality)
		echo		
		echo "*******************************************************"
		echo "* TAPAAL Sequential verifying ReachabilityCardinality *"
		echo "*******************************************************"
                LIST=$(getlist "ReachabilityCardinality.xml")
                verify 7 "$LIST" "-n -s OverApprox" "ReachabilityCardinality.xml"
                verify 40 "$LIST" "-n -d -r 1" "ReachabilityCardinality.xml"
                verify 30 "$LIST" "-n -d -r 1 -s BFS" "ReachabilityCardinality.xml"
                verify 500 "$LIST" "-n -d -r 1 -s DFS" "ReachabilityCardinality.xml"
                exit 0 
		;;

	ReachabilityFireability)
		echo		
		echo "*******************************************************"
		echo "* TAPAAL Sequential verifying ReachabilityFireability *"
		echo "*******************************************************"
                LIST=$(getlist "ReachabilityFireability.xml")
                verify 5 "$LIST" "-n -s OverApprox" "ReachabilityFireability.xml"
                verify 40 "$LIST" "-n -d -r 1" "ReachabilityFireability.xml"
                verify 30 "$LIST" "-n -d -r 1 -s BFS" "ReachabilityFireability.xml"
                verify 500 "$LIST" "-n -d -r 1 -s DFS" "ReachabilityFireability.xml"
                exit 0    
		;;

	CTLCardinality)
		echo		
		echo "**********************************************"
		echo "* TAPAAL Sequential verifying CTLCardinality *"
		echo "**********************************************"
                LIST=$(getlist "CTLCardinality.xml")
                verify 20 "$LIST" "-n -d -ctl czero -s DFS" "CTLCardinality.xml"
                verify 20 "$LIST" "-n -d -ctl czero -s BFS" "CTLCardinality.xml"
                verify 500 "$LIST" "-n -d -ctl czero -s DFS" "CTLCardinality.xml"
                exit 0    
		;;

	CTLFireability)
		echo		
		echo "**********************************************"
		echo "* TAPAAL Sequential verifying CTLFireability *"
		echo "**********************************************"
                LIST=$(getlist "CTLFireability.xml")
                verify 20 "$LIST" "-n -d -ctl czero -s DFS" "CTLFireability.xml"
                verify 20 "$LIST" "-n -d -ctl czero -s BFS" "CTLFireability.xml"
                verify 500 "$LIST" "-n -d -ctl czero -s DFS" "CTLFireability.xml"
                exit 0    
		;;

	*)
		echo "DO_NOT_COMPETE"	
		exit 0
		;;
esac
