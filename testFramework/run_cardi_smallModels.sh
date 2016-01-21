#!/bin/bash
RUNPATH=`pwd`

MODELFILE="./model.pnml"
ENGINE="-ctl"
TOOL="../../DGEngine_singleQuery.sh"
ALGORITHM=("czero-i")
STRATEGY=("CDFS")
ANALYSE="analyse.sh"
MEMORY="mem.sh"

REL_PROGRAMPATH="../verifypn-linux64"
INPUTSPATH="$RUNPATH/smallModels/"
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

                for QF in $(find -mindepth 1 -maxdepth 1 -type f -name 'CTLCard*.xml') ; do

                        qfile="$(basename $QF)"
                        echo "$qfile"
                        export qname="${qfile%.*}"

                        for A in ${ALGORITHM[@]}; do
                                export aname="$A"
                                echo "$A"


                                for S in ${STRATEGY[@]}; do
                                        export sname="$S"
                                        echo "$S"

                                        if [[ ("$S" == CDFS && "$A" != "czero-i") ]]; then
                                                echo"incompatible algorithm and search strategy"
                                        else
                                        		./$TOOL $PROGRAMPATH $MODELFILE $QF $ENGINE 1 & ./$TOOL $PROGRAMPATH $MODELFILE $QF $ENGINE 2 & ./$TOOL $PROGRAMPATH $MODELFILE $QF $ENGINE 3 & ./$TOOL $PROGRAMPATH $MODELFILE $QF $ENGINE 4 & ./$TOOL $PROGRAMPATH $MODELFILE $QF $ENGINE 5 & ./$TOOL $PROGRAMPATH $MODELFILE $QF $ENGINE 6 & ./$TOOL $PROGRAMPATH $MODELFILE $QF $ENGINE 7 & ./$TOOL $PROGRAMPATH $MODELFILE $QF $ENGINE 8 & ./$TOOL $PROGRAMPATH $MODELFILE $QF $ENGINE 9 & ./$TOOL $PROGRAMPATH $MODELFILE $QF $ENGINE 10 & ./$TOOL $PROGRAMPATH $MODELFILE $QF $ENGINE 11 & ./$TOOL $PROGRAMPATH $MODELFILE $QF $ENGINE 12 & ./$TOOL $PROGRAMPATH $MODELFILE $QF $ENGINE 13 & ./$TOOL $PROGRAMPATH $MODELFILE $QF $ENGINE 14 & ./$TOOL $PROGRAMPATH $MODELFILE $QF $ENGINE 15 & ./$TOOL $PROGRAMPATH $MODELFILE $QF $ENGINE 16
						wait
                                                ##{ ./$MEMORY; } > "$mname-$qname-$aname-$sname-mem.log" 
                                                #cat "$mname-$qname-$aname-$sname.log" "$mname-$qname-$aname-$sname-mem.log" >> "$mname-$qname-$aname-$sname-all.log"
                                                #rm "$mname-$qname-$aname-$sname-mem.log"
                                                mv *.log ../../testResults/
                                        fi

                                done
                        done
                 done
        else
                echo "Cannot find $MODELFILE in $MODELPATH"
        fi

        #remove scripts after usage
#       rm $TOOL
        #rm $MEMORY
        cd ../..
        #for log in $(find -mindepth 1 -maxdepth 1 -type f -name '*.log'); do
        #       mv "$log" "$RESULTPATH"
        #done

done

unset qname
unset mname
unset aname
