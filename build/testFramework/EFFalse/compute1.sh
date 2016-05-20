#!/bin/sh
#SBATCH --no-requeue
#SBATCH --mem 1000000
#SBATCH --nodes=1
#SBATCH --partition=production
#SBATCH --ntasks=1
#SBATCH --exclusive
#SBATCH --mail-type=ALL # Type of email notification- BEGIN,END,FAIL,ALL
#SBATCH --mail-user={d803f16@cs.aau.dk}
#SBATCH --time=4
#SBATCH --nodelist=compute1

MODEL=$1
WORKERS=$2
TIMEOUT=$3
RUN_NO=$4
OUTPUTFILE="$MODEL"-EFFalse-W"$WORKERS"-T"$TIMEOUT"-R"$RUN_NO".log

mkdir ~/results
mkdir ~/results/master
mkdir ~/results/master/EFFalse
mkdir ~/results/master/EFFalse/W"$WORKERS"-R"$RUN_NO"

ulimit -S -v 1024000000
ulimit -l 1024000000

export MAXMEM_KB=16000000
{ timeout "$TIMEOUT" /user/smni12/launchpad/master/verifypn-linux64 /user/smni12/launchpad/modelDatabase/allModels/"$MODEL"/model.pnml /user/smni12/launchpad/master/build/testFramework/EF-False.xml -ctl czero -s DFS -x 1; } >> ~/results/master/EFFalse/W"$WORKERS"-R"$RUN_NO"/$OUTPUTFILE 2>&1

ulimit -S -v unlimited

echo Successful result move
echo $SLURM_NODELIST
echo $SLURM_NPROCS
pwd
hostname
rm -- "$0"
