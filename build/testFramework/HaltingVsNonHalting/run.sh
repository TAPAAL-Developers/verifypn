#!/bin/bash
RUN_NO="$1"
max_timeout=60
BINARY=verifypn-linux64-nonhalting

# Compute 1
for binary in verifypn-linux64-nonhalting verifypn-linux64;
do
    for worker_count in 1 2 4 8 16 32 48 64;
    do
            let timeout="$max_timeout"
            sbatch /user/smni12/launchpad/master/build/testFramework/HaltingVsNonHalting/compute1.sh "$worker_count" "$timeout" "$BINARY"-"$RUN_NO" "$BINARY"
    done
done
