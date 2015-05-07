#!/bin/bash
# This script calls LTSmin with the created successor generator shared object file.

# *************** LTSMIN SETTINGS ************************
# Setting for query: --invariant="! goal" 
# Setting for tracelog: --trace=solution.gcf
# Setting for deadlock: --deadlock
# Setting for benchmark-test: --when
# Setting for use of table instead of tree: --state=table

# Pretty Print tracelog: ltsmin-printtrace solution.gcf
# ********************************************************

# Expect -mc to run ltsmin multicore (default sequential)

#PREFIX=/home/mads/verifypnLTSmin
#PREFIX=/home/isabella/Documents/verifypnLTSmin
PREFIX=/Users/user/ikaufm12/verifypnLTSmin

STRAT=$1
NRCORES=$2
ISDEADLOCK=$3
LTSMINTYPE=$4

if [[ $OSTYPE == linux* ]]
then
    LTSMINPATH="$PREFIX/lib/LTSmin/bin/"
elif [[ $OSTYPE == darwin* ]]
then
    LTSMINPATH="$PREFIX/lib/ltsmin.osx64/"
    CFLAGS="-undefined dynamic_lookup"
fi

gcc -O3 -c -I $PREFIX/lib/LTSmin/include/ -I $PREFIX/autogenerated -std=c99 -fPIC $PREFIX/autogenerated/dlopen-impl.c -o $PREFIX/autogenerated/dlopen-impl.o
gcc -w -c -I $PREFIX/lib/LTSmin/include -I $PREFIX/autogenerated -std=c99 -fPIC $PREFIX/autogenerated/autogeneratedfile.c -o $PREFIX/autogenerated/autogeneratedfile.o
gcc -shared -o $PREFIX/autogenerated/sharedfile.so $CFLAGS $PREFIX/autogenerated/dlopen-impl.o $PREFIX/autogenerated/autogeneratedfile.o

if [[ $LTSMINTYPE == "-mc" ]]
then

    if [[ $ISDEADLOCK != "true" ]] 
    then
    	${LTSMINPATH}pins2lts-mc --strategy=$STRAT --procs=$NRCORES --size=22 --state=table  --invariant="! goal" --when $PREFIX/autogenerated/sharedfile.so
    else
	${LTSMINPATH}pins2lts-mc --strategy=$STRAT --procs=$NRCORES --state=table --deadlock --when $PREFIX/autogenerated/sharedfile.so 
    fi

else
    if [[ $ISDEADLOCK != "true" ]] 
    then
        ${LTSMINPATH}pins2lts-seq --strategy=dfs --state=table   --cache-ratio=1  --invariant="! goal" --when $PREFIX/autogenerated/sharedfile.so 
    else
	${LTSMINPATH}pins2lts-seq --strategy=dfs --state=table --deadlock --when $PREFIX/autogenerated/sharedfile.so
    fi
fi
