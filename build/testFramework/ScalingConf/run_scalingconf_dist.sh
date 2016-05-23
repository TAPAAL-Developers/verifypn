#!/bin/bash
RUN_NO=$1
max_timeout=1800

# Compute 1
# Kanban-PT-0005 ResAllocation-PT-R003C010 BridgeAndVehicles-PT-V20P10N10
for model in Kanban-PT-0005 ResAllocation-PT-R003C010 BridgeAndVehicles-PT-V20P10N10;
do
    for worker_count in 1 2 4 8 16 32 48 64;
    do
        for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16;
        do
            let timeout="$max_timeout"
            sbatch /user/smni12/launchpad/master/build/testFramework/ScalingConf/compute1.sh "$model" "$worker_count" "$timeout" "$i" "$RUN_NO"
        done
    done
done

# Compute 2
# HouseConstruction-PT-005 ParamProductionCell-PT-4 ParamProductionCell-PT-5
for model in HouseConstruction-PT-005 ParamProductionCell-PT-4 ParamProductionCell-PT-5;
do
    for worker_count in 1 2 4 8 16 32 48 64;
    do
        for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16;
        do
            let timeout="$max_timeout"
            sbatch /user/smni12/launchpad/master/build/testFramework/ScalingConf/compute2.sh "$model" "$worker_count" "$timeout" "$i" "$RUN_NO"
        done
    done
done

# Compute 3
# NeoElection-PT-3 Peterson-PT-3
for model in NeoElection-PT-3 Peterson-PT-3;
do
    for worker_count in 1 2 4 8 16 32 48 64;
    do
        for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16;
        do
            let timeout="$max_timeout"
            sbatch /user/smni12/launchpad/master/build/testFramework/ScalingConf/compute3.sh "$model" "$worker_count" "$timeout" "$i" "$RUN_NO"
        done
    done
done

# Compute 4
# BridgeAndVehicles-PT-V20P20N20
for model in BridgeAndVehicles-PT-V20P20N20;
do
    for worker_count in 1 2 4 8 16 32 48 64;
    do
        for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16;
        do
            let timeout="$max_timeout"
            sbatch /user/smni12/launchpad/master/build/testFramework/ScalingConf/compute4.sh "$model" "$worker_count" "$timeout" "$i" "$RUN_NO"
        done
    done
done

# Compute 5
# SharedMemory-PT-000010
for model in SharedMemory-PT-000010;
do
    for worker_count in 1 2 4 8 16 32 48 64;
    do
        for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16;
        do
            let timeout="$max_timeout"
            sbatch /user/smni12/launchpad/master/build/testFramework/ScalingConf/compute5.sh "$model" "$worker_count" "$timeout" "$i" "$RUN_NO"
        done
    done
done

# Compute 6
# Philosophers-PT-002000
for model in Philosophers-PT-002000;
do
    for worker_count in 1 2 4 8 16 32 48 64;
    do
        for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16;
        do
            let timeout="$max_timeout"
            sbatch /user/smni12/launchpad/master/build/testFramework/ScalingConf/compute6.sh "$model" "$worker_count" "$timeout" "$i" "$RUN_NO"
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

# Compute 9
# HypercubeGrid-PT-C4K3P3B12
for model in HypercubeGrid-PT-C4K3P3B12;
do
    for worker_count in 1 2 4 8 16 32 48 64;
    do
        for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16;
        do
            let timeout="$max_timeout"
            sbatch /user/smni12/launchpad/master/build/testFramework/ScalingConf/compute9.sh "$model" "$worker_count" "$timeout" "$i" "$RUN_NO"
        done
    done
done

# Solitaire-PT-SqrNC5x5
# Compute [1,2,4,5,6,7,8,9]
for model in Solitaire-PT-SqrNC5x5;
do
    for worker_count in 1 2 4 8 16 32 48 64;
    do
        # Compute 7
        for i in 1 2;
        do
            let timeout="$max_timeout"
            sbatch /user/smni12/launchpad/master/build/testFramework/ScalingConf/compute7.sh "$model" "$worker_count" "$timeout" "$i" "$RUN_NO"
        done

        # Compute 5
        for i in 3 4;
        do
            let timeout="$max_timeout"
            sbatch /user/smni12/launchpad/master/build/testFramework/ScalingConf/compute5.sh "$model" "$worker_count" "$timeout" "$i" "$RUN_NO"
        done

	# Compute 9
        for i in 5 6;
        do
            let timeout="$max_timeout"
            sbatch /user/smni12/launchpad/master/build/testFramework/ScalingConf/compute9.sh "$model" "$worker_count" "$timeout" "$i" "$RUN_NO"
        done

        # Compute 6
        for i in 7 8;
        do
            let timeout="$max_timeout"
            sbatch /user/smni12/launchpad/master/build/testFramework/ScalingConf/compute6.sh "$model" "$worker_count" "$timeout" "$i" "$RUN_NO"
        done

        # Compute 4
        for i in 9 10;
        do
            let timeout="$max_timeout"
            sbatch /user/smni12/launchpad/master/build/testFramework/ScalingConf/compute4.sh "$model" "$worker_count" "$timeout" "$i" "$RUN_NO"
        done
	
        # Compute 8
        for i in 11 12;
        do
            let timeout="$max_timeout"
            sbatch /user/smni12/launchpad/master/build/testFramework/ScalingConf/compute8.sh "$model" "$worker_count" "$timeout" "$i" "$RUN_NO"
        done

        # Compute 2
        for i in 13 14;
        do
            let timeout="$max_timeout"
            sbatch /user/smni12/launchpad/master/build/testFramework/ScalingConf/compute2.sh "$model" "$worker_count" "$timeout" "$i" "$RUN_NO"
        done

        # Compute 1
        for i in 15 16;
        do
            let timeout="$max_timeout"
            sbatch /user/smni12/launchpad/master/build/testFramework/ScalingConf/compute1.sh "$model" "$worker_count" "$timeout" "$i" "$RUN_NO"
        done
    done
done
