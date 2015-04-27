    export PREFIX=/home/mads/MCC15/Scripts
    TOOL=BenchKit_head.sh
    INPUTSPATH=/home/mads/MCC15/INPUTS

for D in $(find ${INPUTSPATH} -mindepth 1 -maxdepth 1 -type d) ; do
    echo;
    echo "####################################################################";
    echo $D ;
    cp $PREFIX/BenchKit_head.sh $D;
    cd $D ;

    #for TOOLNAME in classicSEQ classicMC ontheflySEQ ontheflyMC ontheflyPAR; do
    for TOOLNAME in ontheflyPAR ontheflyMC; do
      export BK_TOOL=$TOOLNAME ;
      
      #for EXAMINATION in StateSpace ReachabilityComputeBounds ReachabilityDeadlock \
      #                   ReachabilityCardinality ReachabilityFireability \
      #                   ReachabilityFireabilitySimple; do

      for EXAMINATION in ReachabilityCardinality; do
         export BK_EXAMINATION=$EXAMINATION ;
         time ./$TOOL
      done 
    done 
    cd ../..
done
