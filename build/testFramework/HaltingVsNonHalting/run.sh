#!/bin/bash
RUN_NO="$1"
max_timeout=60
BINARY=verifypn-linux64-nonhalting

# Compute 1
for BINARY in verifypn-linux64 verifypn-linux64-nonhalting;
do
    for worker_count in 1 2 4 8 16 32 48 64;
    do
            let timeout="$max_timeout"
            sbatch /user/smni12/launchpad/master/build/testFramework/HaltingVsNonHalting/compute1.sh "$worker_count" "$timeout" "$RUN_NO" "$BINARY"
    done
done
