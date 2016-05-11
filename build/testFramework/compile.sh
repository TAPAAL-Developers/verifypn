#!/bin/sh
#SBATCH --nodes 1
#SBATCH --partition=production
#SBATCH -n 1

make -C $1 clean
make -C $1 -f makefile.linux64 -j 64

echo $SLURM_NODELIST
echo $SLURM_NPROCS
pwd
hostname
