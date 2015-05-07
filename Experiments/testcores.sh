#export PREFIX=/home/mads/verifypnLTSmin/Scripts
export PREFIX=/Users/dyhr/Bazaar/verifypnLTSmin/Experiments

#INPUTSPATH=/home/mads/INPUTS
INPUTSPATH=/Users/dyhr/Bazaar/verifypnLTSmin/INPUT_MC

TOOL=BenchKit_head.sh
	
 # CACHE_SIZE=$(cat /sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size)

 #     if [[ $CACHE_SIZE  !=  64 ]]
 #     then
 #         echo "!!!******************************************************************!!!"
 #         echo "  =============INCORRECT ARCHITECTURE - NOT 64-bit CACHE=============="
 #         echo "!!!******************************************************************!!!"
 #         echo "!!!******************************************************************!!!"
 #         echo "!!!************ Please contact developers to setup ******************!!!"
 #         echo "!!!************ and compile the correct version.   ******************!!!"
 #         echo "!!!************ Please also provide the actual     ******************!!!"
 #         echo "!!!************ cache size.                        ******************!!!"
 #         echo "!!!************ ACTUAL CACHE SIZE:  $CACHE_SIZE    ******************!!!"
 #         echo "!!!******************************************************************!!!"
 #         echo "!!!******************************************************************!!!"
 #         exit
 #     fi

for D in $(find ${INPUTSPATH} -mindepth 1 -maxdepth 1 -type d) ; do
    echo;
    echo "####################################################################";
    echo "#MODEL "$D ;
    cp $PREFIX/BenchKit_head.sh $D;
    cd $D ;

    #for TOOLNAME in classicSEQ classicMC ontheflySEQ ontheflyMC ontheflyPAR; do
    for TOOLNAME in ontheflyPAR; do
      export BK_TOOL=$TOOLNAME ;
      
      #for EXAMINATION in StateSpace ReachabilityComputeBounds ReachabilityDeadlock \
      #                   ReachabilityCardinality ReachabilityFireability \
      #                   ReachabilityFireabilitySimple; do


      for EXAMINATION in StateSpace; do

         export BK_EXAMINATION=$EXAMINATION ;
         for cores in 2 4 6 8; do
            export CORES=$cores;
            time ./$TOOL
         done
      done 
    done 
    cd ../..
done
