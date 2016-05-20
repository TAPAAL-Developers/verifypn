#!/bin/bash
ALG=$1
RUN_NO=$2
max_timeout=1800

# Compute 1
# Kanban-PT-0005 ResAllocation-PT-R003C010 BridgeAndVehicles-PT-V20P10N10
for model in Kanban-PT-0005 ResAllocation-PT-R003C010 BridgeAndVehicles-PT-V20P10N10;
do
    for worker_count in 1;
    do
        for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16;
        do
            let timeout="$max_timeout"
            sbatch /user/smni12/launchpad/master/build/testFramework/ScalingConf/compute1_seq.sh "$model" "$ALG" "$timeout" "$i" "$RUN_NO"
        done
    done
done

# Compute 2
# HouseConstruction-PT-005 ParamProductionCell-PT-4 ParamProductionCell-PT-5
for model in HouseConstruction-PT-005 ParamProductionCell-PT-4 ParamProductionCell-PT-5;
do
    for worker_count in 1;
    do
        for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16;
        do
            let timeout="$max_timeout"
            sbatch /user/smni12/launchpad/master/build/testFramework/ScalingConf/compute2_seq.sh "$model" "$ALG" "$timeout" "$i" "$RUN_NO"
        done
    done
done

# Compute 3
# NeoElection-PT-3 Peterson-PT-3
for model in NeoElection-PT-3 Peterson-PT-3;
do
    for worker_count in 1;
    do
        for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16;
        do
            let timeout="$max_timeout"
            sbatch /user/smni12/launchpad/master/build/testFramework/ScalingConf/compute3_seq.sh "$model" "$ALG" "$timeout" "$i" "$RUN_NO"
        done
    done
done

# Compute 4
# BridgeAndVehicles-PT-V20P20N20
for model in BridgeAndVehicles-PT-V20P20N20;
do
    for worker_count in 1;
    do
        for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16;
        do
            let timeout="$max_timeout"
            sbatch /user/smni12/launchpad/master/build/testFramework/ScalingConf/compute4_seq.sh "$model" "$ALG" "$timeout" "$i" "$RUN_NO"
        done
    done
done

# Compute 5
# SharedMemory-PT-000010
for model in SharedMemory-PT-000010;
do
    for worker_count in 1;
    do
        for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16;
        do
            let timeout="$max_timeout"
            sbatch /user/smni12/launchpad/master/build/testFramework/ScalingConf/compute5_seq.sh "$model" "$ALG" "$timeout" "$i" "$RUN_NO"
        done
    done
done

# Compute 6
# Philosophers-PT-002000
for model in Philosophers-PT-002000;
do
    for worker_count in 1;
    do
        for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16;
        do
            let timeout="$max_timeout"
            sbatch /user/smni12/launchpad/master/build/testFramework/ScalingConf/compute6_seq.sh "$model" "$ALG" "$timeout" "$i" "$RUN_NO"
        done
    done
done

# Compute 7
# Solitaire-PT-SqrNC5x5
for model in Solitaire-PT-SqrNC5x5;
do
    for worker_count in 1;
    do
        for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16;
        do
            let timeout="$max_timeout"
            sbatch /user/smni12/launchpad/master/build/testFramework/ScalingConf/compute7_seq.sh "$model" "$ALG" "$timeout" "$i" "$RUN_NO"
        done
    done
done

# Compute 8
# BridgeAndVehicles-PT-V20P20N10
for model in Solitaire-PT-SqrNC5x5;
do
    for worker_count in 1;
    do
        for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16;
        do
            let timeout="$max_timeout"
            sbatch /user/smni12/launchpad/master/build/testFramework/ScalingConf/compute8_seq.sh "$model" "$ALG" "$timeout" "$i" "$RUN_NO"
        done
    done
done

# Compute 9
# HypercubeGrid-PT-C4K3P3B12
for model in HypercubeGrid-PT-C4K3P3B12;
do
    for worker_count in 1;
    do
        for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16;
        do
            let timeout="$max_timeout"
            sbatch /user/smni12/launchpad/master/build/testFramework/ScalingConf/compute9_seq.sh "$model" "$ALG" "$timeout" "$i" "$RUN_NO"
        done
    done
done
