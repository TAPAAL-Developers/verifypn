# ARGUMENTS
REPOSITORY=${1:-"../../.."}
OUTPUTSFOLDER=${2:-"./outputs"}
EXECUTABLE=${3:-verifypn-linux64} # linux(default), osx or win
TIMEOUTTOOL=${4:-timeout} # timeout(default) for linux or gtimeout for osx
ALG=${4:-"dist"}
TIMEOUTTOOL=${5:-timeout} # timeout(default) for linux or gtimeout for osx
TIMEOUT=${6:-5}

# GLOBAL VARIABLS
TESTFOLDER=$REPOSITORY/build/testing/consistency
LIBS=$TESTFOLDER/libs

RESOURCES=MCC
RESOURCESFOLDER=$TESTFOLDER/testResources/$RESOURCES

CONSISTENCY_TESTS_ACTUAL=$TESTFOLDER/actualResults/mcc_consistency_tests_actual.csv
CONSISTENCY_TESTS_EXPECTED=$TESTFOLDER/expectedResults/mcc_consistency_tests_expected.csv

echo "####################################"
echo "MCC CONSISTENCY"
echo "####################################"

echo "<-- REMOVE OUTPUT AND RESULTS FROM LAST RUN -->"
	echo "--REMOVE OUTPUT LOG FILES"
	rm $OUTPUTSFOLDER/*.log
	echo "--REMOVE RESULT CSV FILES"
	rm CONSISTENCY_TESTS_ACTUAL

echo "<-- VERIFY MCC MODELS -->"
for MODEL in Angiogenesis-PT-10 BridgeAndVehicles-PT-V10P10N10 CircadianClock-PT-000010 CircularTrains-PT-024 CSRepetitions-COL-02 DatabaseWithMutex-COL-04 Dekker-PT-010 Diffusion2D-PT-D05N050 Echo-PT-d02r19;
do
	echo "$MODEL"
    for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16; 
    do
		OUTPUTFILE=$OUTPUTSFOLDER/"$MODEL"-CTLCardinality-"$i".log
		$LIBS/_run_ctl_cardinality.sh $REPOSITORY $EXECUTABLE $TIMEOUTTOOL $TIMEOUT $MODEL $ALG $RESOURCESFOLDER $i $OUTPUTFILE
	done
done

echo "<-- CONSISTENCY TEST ON RESULTS -->"
echo "--Aggregate log files to one CSV file"
python3 $REPOSITORY/build/aggregate_logs.py $OUTPUTSFOLDER $CONSISTENCY_TESTS_ACTUAL

echo "--Compair actual results with expected results"
python3 $LIBS/consistency_check.py $CONSISTENCY_TESTS_ACTUAL $CONSISTENCY_TESTS_EXPECTED

CONSISTENT=$?

exit $CONSISTENT
