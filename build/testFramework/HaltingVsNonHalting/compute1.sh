#!/bin/sh
#SBATCH --no-requeue
#SBATCH --mem 1000000
#SBATCH --nodes=1
#SBATCH --partition=production
#SBATCH --ntasks=1
#SBATCH --exclusive
#SBATCH --mail-type=ALL # Type of email notification- BEGIN,END,FAIL,ALL
#SBATCH --mail-user={d803f16@cs.aau.dk}
#SBATCH --time=00:30:05
#SBATCH --nodelist=compute1

WORKERS=$1
TIMEOUT=$2
RUN_NO=$3
BINARY=$4
OUTPUTFILE="$BINARY"-CTLCardinality-Cell2-"$I"-W"$WORKERS"-T"$TIMEOUT"-R"$RUN_NO".log

mkdir ~/results
mkdir ~/results/master
mkdir ~/results/master/HaltingVsNonHalting
mkdir ~/results/master/HaltingVsNonHalting/"$BINARY"W"$WORKERS"-R"$RUN_NO"

ulimit -S -v 1024000000
ulimit -l 1024000000

export MAXMEM_KB=16000000
{ timeout "$TIMEOUT" mpirun -np "$WORKERS" /user/smni12/launchpad/master/"$BINARY" /user/smni12/launchpad/modelDatabase/allModels/ParamProductionCell-PT-5/model.pnml /user/smni12/launchpad/modelDatabase/allModels/ParamProductionCell-PT-3/CTLCardinality.xml -ctl dist -s DFS -x 6; } >> ~/results/master/HaltingVsNonHalting/"$BINARY"W"$WORKERS"-R"$RUN_NO"/$OUTPUTFILE 2>&1

ulimit -S -v unlimited

echo Successful result move
echo $SLURM_NODELIST
echo $SLURM_NPROCS
pwd
hostname
rm -- "$0"
