#!/bin/bash
RUN_NO=$1
max_timeout=38400

# DEKKER-PT-015 - Compute 1
for worker_count in 1 2 4 8 16 32 48 64;
do
let timeout="$max_timeout / $worker_count"
     sbatch /user/smni12/launchpad/master/build/testFramework/EFFalse/compute1_dist.sh "Dekker-PT-015" "$worker_count" "$timeout" "$RUN_NO"
done

# IOTPpurchase-PT-C03M03P03D03 - Compute 2
for worker_count in 1 2 4 8 16 32 48 64;
do
let timeout="$max_timeout / $worker_count"
     sbatch /user/smni12/launchpad/master/build/testFramework/EFFalse/compute2_dist.sh "IOTPpurchase-PT-C03M03P03D03" "$worker_count" "$timeout" "$RUN_NO"
done

# ParamProductionCell-PT-2 - Compute 3
for worker_count in 1 2 4 8 16 32 48 64;
do
let timeout="$max_timeout / $worker_count"
     sbatch /user/smni12/launchpad/master/build/testFramework/EFFalse/compute3_dist.sh "ParamProductionCell-PT-2" "$worker_count" "$timeout" "$RUN_NO"
done

# PhilosophersDyn-PT-10 - Compute 4
for worker_count in 1 2 4 8 16 32 48 64;
do
let timeout="$max_timeout / $worker_count"
     sbatch /user/smni12/launchpad/master/build/testFramework/EFFalse/compute4_dist.sh "PhilosophersDyn-PT-10" "$worker_count" "$timeout" "$RUN_NO"
done

# RwMutex-PT-r0010w2000 - Compute 5
for worker_count in 1 2 4 8 16 32 48 64;
do
let timeout="$max_timeout / $worker_count"
     sbatch /user/smni12/launchpad/master/build/testFramework/EFFalse/compute5_dist.sh "RwMutex-PT-r0010w2000" "$worker_count" "$timeout" "$RUN_NO"
done

# TokenRing-PT-010 - Compute 6
for worker_count in 1 2 4 8 16 32 48 64;
do
let timeout="$max_timeout / $worker_count"
     sbatch /user/smni12/launchpad/master/build/testFramework/EFFalse/compute6_dist.sh "TokenRing-PT-010" "$worker_count" "$timeout" "$RUN_NO"
done

# ResAllocation-PT-R003C010 - Compute 7
for worker_count in 1 2 4 8 16 32 48 64;
do
let timeout="$max_timeout / $worker_count"
     sbatch /user/smni12/launchpad/master/build/testFramework/EFFalse/compute7_dist.sh "ResAllocation-PT-R003C010" "$worker_count" "$timeout" "$RUN_NO"
done

# ParamProductionCell-PT-3 - Compute 8
for worker_count in 1 2 4 8 16 32 48 64;
do
let timeout="$max_timeout / $worker_count"
     sbatch /user/smni12/launchpad/master/build/testFramework/EFFalse/compute8_dist.sh "ParamProductionCell-PT-3" "$worker_count" "$timeout" "$RUN_NO"
done

# SwimmingPool-PT-02 - Compute 9
for worker_count in 1 2 4 8 16 32 48 64;
do
let timeout="$max_timeout / $worker_count"
     sbatch /user/smni12/launchpad/master/build/testFramework/EFFalse/compute9_dist.sh "SwimmingPool-PT-02" "$worker_count" "$timeout" "$RUN_NO"
done
