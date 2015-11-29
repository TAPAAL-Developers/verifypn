#!/bin/bash
RUNPATH=`pwd`

MODELFILE="./model.pnml"
ENGINE="-ctl"
TOOL="DGEngine.sh"
ALGORITHM=("local" "global" "czero")
STRATEGY=("BFS" "DFS")
ANALYSE="analyse.sh"
MEMORY="mem.sh"
TIMEOUT=3600

REL_PROGRAMPATH="../verifypn-linux64"
INPUTSPATH="$RUNPATH/testModels/"
RESULTPATH="$RUNPATH/testResults/"
ANALISEPATH="$RUNPATH/analysis"

if [ -f $REL_PROGRAMPATH ] 
	then
	PROGRAMPATH=$(cd $(dirname $REL_PROGRAMPATH); pwd)/$(basename $REL_PROGRAMPATH)
else
	echo "Cannot find file $PROGRAMPATH"
	exit 1
fi

echo "####################################################################";
echo "######################### TEST INFORMATION #########################";
echo "####################################################################";
echo "|| Program path: $PROGRAMPATH";
echo "|| Inputs path:  $INPUTSPATH";
echo "|| Result path:  $RESULTPATH";
echo "####################################################################";
echo "";
echo "Running tests... Please wait";

for D in $(find ${INPUTSPATH} -mindepth 1 -maxdepth 1 -type d) ; do
    echo;
    echo "####################################################################";
    echo $D ;
    cp $TOOL $D
    cp $MEMORY $D
    cd $D ;
    
	export mname="`basename "$PWD"`"

	#verify model exists
	if [ -f $MODELFILE ] ; then

		for QF in $(find -mindepth 1 -maxdepth 1 -type f -name 'CTL*.xml') ; do

		 	qfile="$(basename $QF)"
		 	echo "$qfile"
			export qname="${qfile%.*}"

			for A in ${ALGORITHM[@]}; do
				export aname="$A"
				echo "$A"

				for S in ${STRATEGY[@]}; do
					export sname="$S"
					echo "$S"

					#<PROGRAMPATH> <MODELFILE> <QUERYFILE> <ENGINE> <ALGORITHM>
				 	timeout $TIMEOUT ./$TOOL $PROGRAMPATH $MODELFILE $QF $ENGINE&
				 	{ ./$MEMORY; } > "$mname-$qname-$aname-$sname-mem.log" 
				 	cat "$mname-$qname-$aname-$sname.log" "$mname-$qname-$aname-$sname-mem.log" >> "$mname-$qname-$aname-$sname-all.log"
				 	rm "$mname-$qname-$aname-$sname.log" 
				 	rm "$mname-$qname-$aname-$sname-mem.log"
				 	mv $mname-$qname-$aname-$sname-all.log ../../testResults/$mname-$qname-$aname-$sname.log
			 	done
			done

		 done

	else
		echo "Cannot find $MODELFILE in $MODELPATH"
	fi

	#for log in $(find -mindepth 1 -maxdepth 1 -type f -name '*.log'); do
	#	mv "$log" "$RESULTPATH"
	#done

done

unset qname
unset mname
unset aname