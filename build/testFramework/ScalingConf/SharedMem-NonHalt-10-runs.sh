#!/bin/bash
RUN_NO="NONHALT-INVESTIGATE-$1"
max_timeout=600
BINARY=verifypn-linux64-nonhalting

# Compute 4
# SharedMemory-PT-000010

for a in 1 2 3 4 5 6 7 8 9 10;
do
	for model in SharedMemory-PT-000010;
	do
	    for worker_count in 48;
	    do
		for i in 1;
		do
		    let timeout="$max_timeout"
		    sbatch /user/smni12/launchpad/master/build/testFramework/ScalingConf/compute4.sh "$model" "$worker_count" "$timeout" "$i" "$RUN_NO" "$BINARY" "-SHAREDMEM$a"
		done
	    done
	done
done
