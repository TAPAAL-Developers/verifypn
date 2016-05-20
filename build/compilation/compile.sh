MAKEFILEDIR=$1
echo "<-- COMPILING -->"
#!/bin/sh
#SBATCH --nodes 1
#SBATCH --partition=production
#SBATCH -n 1

make -C $MAKEFILEDIR clean
make -C $MAKEFILEDIR -f makefile.linux64 -j 4

echo $SLURM_NODELIST
echo $SLURM_NPROCS
pwd
hostname
