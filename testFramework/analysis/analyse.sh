RESULTFILENAME=$(basename "$1")
RESULTFILENAME="${RESULTFILENAME%%.*}"

./errorAnalysis.sh $1 
./resultAnalysis.sh $1 $RESULTFILENAME
./speedAnalysis.sh $1
./correctnessAnalysis.sh $1

rm $1

./combinelog.sh errorlog.log speedlog.log resultlog.log correctlog.log $RESULTFILENAME
rm errorlog.log 
rm speedlog.log 
rm resultlog.log 
rm correctlog.log
