#!/bin/bash
ALG=$1
RUN_NO=$2
worker_count=1

max_timeout=240

# DEKKER-PT-015 - Compute 1
let timeout="$max_timeout / $worker_count"
sbatch /user/smni12/launchpad/master/build/testFramework/EFFalse/compute1.sh "Dekker-PT-015" "$worker_count" "$timeout" "$ALG" "$RUN_NO"

# IOTPpurchase-PT-C03M03P03D03 - Compute 2
let timeout="$max_timeout / $worker_count"
sbatch /user/smni12/launchpad/master/build/testFramework/EFFalse/compute2.sh "IOTPpurchase-PT-C03M03P03D03" "$worker_count" "$timeout" "$ALG" "$RUN_NO"

# ParamProductionCell-PT-2 - Compute 3
let timeout="$max_timeout / $worker_count"
sbatch /user/smni12/launchpad/master/build/testFramework/EFFalse/compute3.sh "ParamProductionCell-PT-2" "$worker_count" "$timeout" "$ALG" "$RUN_NO"

# PhilosophersDyn-PT-10 - Compute 4
let timeout="$max_timeout / $worker_count"
     sbatch /user/smni12/launchpad/master/build/testFramework/EFFalse/compute4.sh "PhilosophersDyn-PT-10" "$worker_count" "$timeout" "$ALG" "$RUN_NO"

# RwMutex-PT-r0010w2000 - Compute 5
let timeout="$max_timeout / $worker_count"
sbatch /user/smni12/launchpad/master/build/testFramework/EFFalse/compute5.sh "RwMutex-PT-r0010w2000" "$worker_count" "$timeout" "$ALG" "$RUN_NO"

# TokenRing-PT-010 - Compute 6
let timeout="$max_timeout / $worker_count"
sbatch /user/smni12/launchpad/master/build/testFramework/EFFalse/compute6.sh "TokenRing-PT-010" "$worker_count" "$timeout" "$ALG" "$RUN_NO"

# ResAllocation-PT-R003C010 - Compute 7
let timeout="$max_timeout / $worker_count"
sbatch /user/smni12/launchpad/master/build/testFramework/EFFalse/compute7.sh "ResAllocation-PT-R003C010" "$worker_count" "$timeout" "$ALG" "$RUN_NO"

# ParamProductionCell-PT-3 - Compute 6
let timeout="$max_timeout / $worker_count"
sbatch /user/smni12/launchpad/master/build/testFramework/EFFalse/compute6.sh "ParamProductionCell-PT-3" "$worker_count" "$timeout" "$ALG" "$RUN_NO"

# SwimmingPool-PT-02 - Compute 9
let timeout="$max_timeout / $worker_count"
sbatch /user/smni12/launchpad/master/build/testFramework/EFFalse/compute9.sh "SwimmingPool-PT-02" "$worker_count" "$timeout" "$ALG" "$RUN_NO"
