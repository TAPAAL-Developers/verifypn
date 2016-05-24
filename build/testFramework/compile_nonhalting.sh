#!/bin/sh
#SBATCH --nodes 1
#SBATCH --partition=production
#SBATCH -n 1
#SBATCH --time=10

make -C $1 clean -f makefile.local-nonhalting
make -C $1 -f makefile.local-nonhalting -j 64

echo $SLURM_NODELIST
echo $SLURM_NPROCS
pwd
hostname
