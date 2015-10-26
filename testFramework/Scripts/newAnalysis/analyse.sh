echo "========================================================="
echo "============== TEST FRAMEWORK ANALYSING ================="
echo "========================================================="

##### Test data:
#cd Scripts/newAnalysis
gcc testprog.c
./a.out > testlog.log
##### Remove after 


./errorAnalysis.sh testlog.log
./resultAnalysis.sh testlog.log

./speedAnalysis.sh testlog.log
./correctnessAnalysis.sh testlog.log


rm testlog.log
rm analysis.log

./combinelog.sh errorlog.log speedlog.log resultlog.log correctlog.log 
rm errorlog.log 
rm speedlog.log 
rm resultlog.log 
rm correctlog.log

echo "========================= Done =========================="
