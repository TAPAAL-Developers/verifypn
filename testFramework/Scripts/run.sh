PROGRAMPATH=$1
MODELS=$2
ENGINE=$3
QUERYTYPE=$4
TIMEOUT=$5

export PREFIX=$PROGRAMPATH/Scripts
export TOUT=$TIMEOUT


INPUTSPATH=$PROGRAMPATH/$MODELS

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
echo "####################################################################";
echo "##########################TEST INFORMATION##########################";
echo "####################################################################";
echo "||Program path: $PROGRAMPATH";
echo "||";
echo "####################################################################";
echo "";
echo "Running tests... Please wait";


for D in $(find ${INPUTSPATH} -mindepth 1 -maxdepth 1 -type d) ; do
    echo;
    echo "####################################################################";
    echo $D ;
    cp $PREFIX/BenchKit_head.sh $D;
    cd $D ;

    #for TOOLNAME in classicSEQ classicMC ontheflySEQ ontheflyMC ontheflyPAR; do
    for TOOLNAME in $ENGINE; do
      export BK_TOOL=$TOOLNAME ;
      
      #for EXAMINATION in StateSpace ReachabilityComputeBounds ReachabilityDeadlock \
      #                   ReachabilityCardinality ReachabilityFireability \
      #                   ReachabilityFireabilitySimple CTLFireabilitySimple CTLFireability CTLCardinality; do


      for EXAMINATION in $QUERYTYPE; do

         export BK_EXAMINATION=$EXAMINATION ;
         time ./$TOOL
      done 
    done 
    cd ../..
done
echo "Done testing";
echo "Starting analisys";

