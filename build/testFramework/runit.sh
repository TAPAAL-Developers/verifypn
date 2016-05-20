#!/bin/sh
#SBATCH --nodes 1
#SBATCH --partition=production
#SBATCH -n 1
#SBATCH --mail-type=ALL # Type of email notification- BEGIN,END,FAIL,ALL
#SBATCH --mail-user={d803f16@cs.aau.dk}


for i in {1..1000}
do
	OUTPUTFILE=run"$i".log
	
	ulimit -S -v unlimited
	{ timeout 60  ~/launchpad/parallel-lockfree/verifypn-linux64 /user/smni12/launchpad/verifypnTestFramework/testFramework/completeModelDB/HouseConstruction-PT-002/model.pnml /user/smni12/launchpad/verifypnTestFramework/testFramework/completeModelDB/HouseConstruction-PT-002/CTLCardinality.xml -ctl czero -s DFS -x 7; } >> $OUTPUTFILE 2>&1
	ulimit -S -v unlimited
	
	mkdir testResults/HaltingExperiment	
	mv $OUTPUTFILE testResults/testResults/HaltingExperiment/$OUTPUTFILE
done
