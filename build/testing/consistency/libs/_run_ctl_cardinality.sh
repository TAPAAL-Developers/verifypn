# Script to be called by other testing scripts.
# Runs a query and moves the log file to the designated place
REPOSITORY=$1
EXECUTABLE=$2 # linux, osx or win
TIMEOUTTOOL=$3
TIMEOUT=$4
MODEL=$5
ALG=$6
RESOURCESFOLDER=$7
i=$8
OUTPUTFILE=$9


MPIRUN=""
if [ "$ALG" = 'dist' ]; then 
    MPIRUN="mpirun -np 4"
fi
export MAXMEM_KB=16000000
{ $TIMEOUTTOOL $TIMEOUT $MPIRUN $REPOSITORY/$EXECUTABLE $RESOURCESFOLDER/$MODEL/model.pnml $RESOURCESFOLDER/$MODEL/CTLCardinality.xml -ctl $ALG -s DFS -x $i -p test; } >> $OUTPUTFILE 2>&1
