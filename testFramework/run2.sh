#!/bin/bash
RUNPATH=`pwd`

MODELFILE="./model.pnml"
ENGINE="-ctl"
TOOL="DGEngine.sh"
ALGORITHM=("czero")
STRATEGY=("DFS")
ANALYSE="analyse.sh"
MEMORY="mem.sh"

REL_PROGRAMPATH="../verifypn-linux64"
INPUTSPATH="$RUNPATH/ModelDB/"
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
    #cp $MEMORY $D
    cd $D ;
    
	export mname="`basename "$PWD"`"

	#verify model exists
	if [ -f $MODELFILE ] ; then

		for QF in $(find -mindepth 1 -maxdepth 1 -type f -name 'ReachabilityCard*.xml') ; do

		 	qfile="$(basename $QF)"
		 	echo "$qfile"
			export qname="${qfile%.*}"

			for A in ${ALGORITHM[@]}; do
				export aname="$A"
				echo "$A"
                                

				for S in ${STRATEGY[@]}; do
					export sname="$S"
					echo "$S"

					if [[ "$S" == CDFS && "$A" != "czero" ]]; then
						echo"incompatible algorithm and search strategy"
					else
						#<PROGRAMPATH> <MODELFILE> <QUERYFILE> <ENGINE> <ALGORITHM>
					 	./$TOOL $PROGRAMPATH $MODELFILE $QF $ENGINE $i
					 	#{ ./$MEMORY; } > "$mname-$qname-$aname-$sname-mem.log" 
					 	#cat "$mname-$qname-$aname-$sname.log" "$mname-$qname-$aname-$sname-mem.log" >> "$mname-$qname-$aname-$sname-all.log"
					 	#rm "$mname-$qname-$aname-$sname-mem.log"
					 	mv $mname-$qname-$aname-$sname.log ../../testResults/$mname-$qname-$aname-$sname.log         
					fi
					                    
			 	done
			done
		 done
	else
		echo "Cannot find $MODELFILE in $MODELPATH"
	fi

	#remove scripts after usage
	rm $TOOL
	rm $MEMORY
	cd ../..
	#for log in $(find -mindepth 1 -maxdepth 1 -type f -name '*.log'); do
	#	mv "$log" "$RESULTPATH"
	#done

done

unset qname
unset mname
unset aname