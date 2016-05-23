# Solitaire-PT-SqrNC5x5
# Compute [1,2,4,5,6,7,8,9]
max_timeout=1800

for RUN_NO in 1;
do
	for model in Solitaire-PT-SqrNC5x5;
	do
	    for worker_count in "czero" "local";
	    do
		# Compute 7
		for i in 1 2;
		do
		    let timeout="$max_timeout"
		    sbatch /user/smni12/launchpad/master/build/testFramework/ScalingConf/compute7_seq.sh "$model" "$worker_count" "$timeout" "$i" "$RUN_NO"
		done

		# Compute 5
		for i in 3 4;
		do
		    let timeout="$max_timeout"
		    sbatch /user/smni12/launchpad/master/build/testFramework/ScalingConf/compute5_seq.sh "$model" "$worker_count" "$timeout" "$i" "$RUN_NO"
		done

		# Compute 9
		for i in 5 6;
		do
		    let timeout="$max_timeout"
		    sbatch /user/smni12/launchpad/master/build/testFramework/ScalingConf/compute9_seq.sh "$model" "$worker_count" "$timeout" "$i" "$RUN_NO"
		done

		# Compute 6
		for i in 7 8;
		do
		    let timeout="$max_timeout"
		    sbatch /user/smni12/launchpad/master/build/testFramework/ScalingConf/compute6_seq.sh "$model" "$worker_count" "$timeout" "$i" "$RUN_NO"
		done

		# Compute 4
		for i in 9 10;
		do
		    let timeout="$max_timeout"
		    sbatch /user/smni12/launchpad/master/build/testFramework/ScalingConf/compute4_seq.sh "$model" "$worker_count" "$timeout" "$i" "$RUN_NO"
		done
	
		# Compute 8
		for i in 11 12;
		do
		    let timeout="$max_timeout"
		    sbatch /user/smni12/launchpad/master/build/testFramework/ScalingConf/compute8_seq.sh "$model" "$worker_count" "$timeout" "$i" "$RUN_NO"
		done

		# Compute 2
		for i in 13 14;
		do
		    let timeout="$max_timeout"
		    sbatch /user/smni12/launchpad/master/build/testFramework/ScalingConf/compute2_seq.sh "$model" "$worker_count" "$timeout" "$i" "$RUN_NO"
		done

		# Compute 1
		for i in 15 16;
		do
		    let timeout="$max_timeout"
		    sbatch /user/smni12/launchpad/master/build/testFramework/ScalingConf/compute1_seq.sh "$model" "$worker_count" "$timeout" "$i" "$RUN_NO"
		done
	    done
	done
done
