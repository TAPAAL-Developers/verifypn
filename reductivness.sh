#!/bin/bash
#echo "CircularTrains-PT-012" | ts
#time ./verifypn-linux64 -x 1 -l 1 -s OverApprox ../models/CircularTrains-PT-012/model.pnml ../models/CircularTrains-PT-012/ReachabilityCardinality.xml

for modelname in  HouseConstruction-PT-002
do
    for query in ReachabilityCardinality ReachabilityFireability ReachabilityFireabilitySimple
    do 
        ./verifypn-linux64 -o mc -r 2 -b ../models/$modelname/model.pnml ../models/$modelname/$query.xml
        echo $?
        if [ $? = 4 ] ; then
            echo "This should work!"
        fi
    done
done