# ARGUMENTS
REPOSITORY=${1:-"../../.."}
OUTPUTSFOLDER=${2:-"./outputs"}
EXECUTABLE=${3:-verifypn-linux64} # linux(default), osx or win
ALG=${4:-"dist"}
TIMEOUTTOOL=${5:-timeout} # timeout(default) for linux or gtimeout for osx
TIMEOUT=${6:-5}

# GLOBAL VARIABLS
TESTFOLDER=$REPOSITORY/build/testing/consistency
LIBS=$TESTFOLDER/libs

RESOURCES=CTLUnitTest
RESOURCESFOLDER=$TESTFOLDER/testResources/$RESOURCES

UNIT_TESTS_ACTUAL=$TESTFOLDER/actualResults/ctl_unit_tests_actual.csv
UNIT_TESTS_EXPECTED=$TESTFOLDER/expectedResults/ctl_unit_tests_expected.csv

echo "####################################"
echo "UNIT TEST - CONSISTENCY OF CTL EDGE CASES"
echo "####################################"

echo "<-- REMOVE OUTPUT AND RESULTS FROM LAST RUN -->"
	echo "--REMOVE OUTPUT LOG FILES"
	rm $OUTPUTSFOLDER/*.log
	echo "--REMOVE RESULT CSV FILES"
	rm $UNIT_TESTS_ACTUAL

echo "<-- VERIFY UNIT TEST MODELS -->"
for MODEL in CorrectnessHard CorrectnessNet CorrectnessSimple;
do
	echo "$MODEL"
	for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14; 
	do
		if [ "$MODEL" != "CorrectnessSimple" ] || [ $i -le 12 ]; # CorrectnessSimple has only 12 queries
		then
			OUTPUTFILE=$OUTPUTSFOLDER/UNITTEST_CTLCardinality_"$MODEL""$i".log
			$LIBS/_run_ctl_cardinality.sh $REPOSITORY $EXECUTABLE $TIMEOUTTOOL $TIMEOUT $MODEL $ALG $RESOURCESFOLDER $i $OUTPUTFILE
		fi
	done
done

echo "<-- CONSISTENCY TEST ON RESULTS -->"
echo "--Aggregate log files to one CSV file"
python3 $REPOSITORY/build/aggregate_logs.py $OUTPUTSFOLDER $UNIT_TESTS_ACTUAL

echo "--Compair actual results with expected results"
python3 $LIBS/consistency_check.py $UNIT_TESTS_ACTUAL $UNIT_TESTS_EXPECTED

CONSISTENT=$?

exit $CONSISTENT
