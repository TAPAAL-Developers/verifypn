#!/bin/bash
RUNPATH=`pwd`
MODELS="testModels"
MODELFILE="model.pnml"
RESULTS="testResults"
ENGINE="-ctl"
TOOL="CTLEngine.sh"
ANALYSE="analyse.sh"
MEM="../../mem.sh"

REL_PROGRAMPATH="../verifypn-linux64"

#FLAG=$1
QUERYTYPE=$2
declare -a QUERY;

#TIMEOUT=$5

INPUTSPATH=$RUNPATH/$MODELS
RESULTPATH=$RUNPATH/$RESULTS
ANALISEPATH="$RUNPATH/analysis"

#export PREFIX=$PROGRAMPATH/Scripts
#export TOUT=$TIMEOUT



 # CACHE_SIZE=$(cat /sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size)

 #     if [[ $CACHE_SIZE  !=  64 ]]
 #     then
 #         echo "!!!******************************************************************!!!"
 #         echo "  =============INCORRECT ARCHITECTURE - NOT 64-bit CACHE=============="
 #         echo "!!!******************************************************************!!!"
 #         echo "!!!******************************************************************!!!"
 #         echo "!!!************ Please contact developers to setup ******************!!!"
 #         echo "!!!************ and compile the correct version.   ******************!!!"
 #         echo "!!!************ Please also provide the actual     ******************!!!"
 #         echo "!!!************ cache size.                        ******************!!!"
 #         echo "!!!************ ACTUAL CACHE SIZE:  $CACHE_SIZE    ******************!!!"
 #         echo "!!!******************************************************************!!!"
 #         echo "!!!******************************************************************!!!"
 #         exit
 #     fi

if [ -f $REL_PROGRAMPATH ] 
	then
	PROGRAMPATH=$(cd $(dirname $REL_PROGRAMPATH); pwd)/$(basename $REL_PROGRAMPATH)
else
	echo "Cannot find file $PROGRAMPATH"
	exit 1
fi

# case $QUERYTYPE in
# 	All ) 
# 		echo "Running all supported queries"
# 		QUERY[0]="CTLFireabilitySimple"
# 		QUERY[1]="CTLFireability"
# 		QUERY[2]="CTLCardinality"
# 	;;
# 	CTLFireabilitySimple)
# 		echo "QUERYTYPE: CTLFireabilitySimple"
# 		QUERY[0]="CTLFireabilitySimple"
# 	;;
# 	CTLFireability)
# 		echo "QUERYTYPE: CTLFireability"
# 		QUERY[0]="CTLFireability"
# 	;;
# 	CTLCardinality)
# 		echo "QUERYTYPE: CTLCardinality"
# 		QUERY[0]="CTLCardinality"
# 	;;
# 	* )
# 	echo "Error: invalid query type"
# 	exit 1
# 	;;
# esac

echo "####################################################################";
echo "##########################TEST INFORMATION##########################";
echo "####################################################################";
echo "|| Program path: $PROGRAMPATH";
echo "|| Inputs path: $INPUTSPATH";
echo "|| Result path: $RESULTPATH";
echo "####################################################################";
echo "";
echo "Running tests... Please wait";

for D in $(find ${INPUTSPATH} -mindepth 1 -maxdepth 1 -type d) ; do
    echo;
    echo "####################################################################";
    echo $D ;
    cp $TOOL $D
    cd $D ;
    
	#verify model exists
	if [ -f $MODELFILE ] ; then

		for QF in $(find -mindepth 1 -maxdepth 1 -type f -name 'ReachabilityCa*.xml') ; do
			#echo "$(basename $QF)"
		
			#<PROGRAMPATH> <ENGINE> <MODELFILE> <QUERYFILE>
			time ./$TOOL $PROGRAMPATH $ENGINE $MODELFILE $QF 
			#$FLAG &
			#./$MEM 2> err.txt
			#rm err.txt	
			#wait
			#sleep 20

		done

	else
		echo "Cannot find matching $MODELFILE in $MODELPATH"
	fi


done

echo "Done testing";
