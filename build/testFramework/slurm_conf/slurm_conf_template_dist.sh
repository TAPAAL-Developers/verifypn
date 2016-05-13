#!/bin/sh
#SBATCH --no-requeue
#SBATCH --mem 1000000
#SBATCH --nodes={NODES}
#SBATCH --partition=production
#SBATCH --ntasks=1
#SBATCH --exclusive
#SBATCH --mail-type=ALL # Type of email notification- BEGIN,END,FAIL,ALL
#SBATCH --mail-user={{d803f16@cs.aau.dk}}
#SBATCH --time={JOBDURATION}

OUTPUTFILE={MODELNAME}-{QUERYCAT}-{QUERY_NUM}-{STRATEGY}.log

ulimit -S -v {MEMLIMIT}
ulimit -l {MEMLIMIT}

export MAXMEM_KB=16000000
{{ timeout {TIMEOUT} mpirun -np {WORKERS} {ENGINEPATH} {MODELPATH} {QUERYPATH} {ALGFLAG} -s {STRATEGY} -x {QUERY_NUM}; }} >> $OUTPUTFILE 2>&1

ulimit -S -v unlimited

echo Exitcode: $? >> $OUTPUTFILE 2>&1
echo Run Complete!
mkdir ~/results
mkdir ~/results/verifypn-dist-ctl
mkdir ~/results/verifypn-dist-ctl/{EXPERIMENT}-{MODELCONF}-{TIMESTAMP}-{ENGINENAME}-M{MEMLIMIT}-T{TIMEOUT}
mv $OUTPUTFILE ~/results/verifypn-dist-ctl/{EXPERIMENT}-{MODELCONF}-{TIMESTAMP}-{ENGINENAME}-M{MEMLIMIT}-T{TIMEOUT}/$OUTPUTFILE
echo Successful result move
echo $SLURM_NODELIST
echo $SLURM_NPROCS
pwd
hostname
rm -- "$0"
~                
