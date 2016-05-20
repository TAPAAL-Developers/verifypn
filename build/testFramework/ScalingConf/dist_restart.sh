#!/bin/bash
echo "!!!DO NOT USE!!!"

RUN_NO=1
max_timeout=1800

# Compute 7
# Solitaire-PT-SqrNC5x5
for model in Solitaire-PT-SqrNC5x5;
do
    for worker_count in 1 2 4 8 16 32 48 64;
    do
        for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16;
        do
            let timeout="$max_timeout"
            sbatch /user/smni12/launchpad/master/build/testFramework/ScalingConf/compute7.sh "$model" "$worker_count" "$timeout" "$i" "$RUN_NO"
        done
    done
done

# Compute 8
# BridgeAndVehicles-PT-V20P20N10
for model in BridgeAndVehicles-PT-V20P20N10;
do
    for worker_count in 1 2 4 8 16 32 48 64;
    do
        for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16;
        do
            let timeout="$max_timeout"
            sbatch /user/smni12/launchpad/master/build/testFramework/ScalingConf/compute8.sh "$model" "$worker_count" "$timeout" "$i" "$RUN_NO"
        done
    done
done
