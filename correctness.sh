#!/bin/bash
#echo "CircularTrains-PT-012" | ts
#time ./verifypn-linux64 -x 1 -l 1 -s OverApprox ../models/CircularTrains-PT-012/model.pnml ../models/CircularTrains-PT-012/ReachabilityCardinality.xml

#echo "CircularTrains-PT-024" | ts
#for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16
#do
#time ./verifypn-linux64 -x $i -l 1 -d ../models/CircularTrains-PT-024/model.pnml ../models/CircularTrains-PT-024/ReachabilityCardinality.xml #> logFiles/CircularTrains24Correctness_$i_d.txt
#done

echo "CircularTrains-PT-024" | ts0
time ./verifypn-linux64 -o mc -x 5 -r 2 -b -d -s OverApprox ../models/CircularTrains-PT-024/model.pnml ../models/CircularTrains-PT-024/ReachabilityCardinality.xml #> logFiles/CircularTrains24Correctness_Multi_d.

#echo "CircularTrains-PT-024" | ts
#for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16
#do
#time ./verifypn-linux64 -x $i -l 1 ../models/CircularTrains-PT-024/model.pnml ../models/CircularTrains-PT-024/ReachabilityCardinality.xml > logFiles/CircularTrains24Correctness_$i.txt
#done

#echo "CircularTrains-PT-024" | ts
#time ./verifypn-linux64 -x 1 -l 2 ../models/CircularTrains-PT-024/model.pnml ../models/CircularTrains-PT-024/ReachabilityCardinality.xml > logFiles/CircularTrains24Correctness_Multi.txt

#echo "EnergyBus" | ts
#time ./verifypn-linux64 -x 2 -l 2 ../models/EnergyBus-PT-none/model.pnml ../models/EnergyBus-PT-none/ReachabilityCardinality.xml > logFiles/EnergyBus_s.txt

#echo "Angiogenesis" | ts
#time ./verifypn-linux64 -x 2 -l 2 ../models/Angiogenesis-PT-01/model.pnml ../models/Angiogenesis-PT-01/ReachabilityCardinality.xml > logFiles/Angiogenesis_s.txt

#echo "CSRepetitions" | ts
#time ./verifypn-linux64 -x 2 -l 2 ../models/CSRepetitions-PT-02/model.pnml ../models/CSRepetitions-PT-02/ReachabilityCardinality.xml > logFiles/CSRepetitions_s.txt

#echo "Eratosthenes" | ts
#time ./verifypn-linux64 -x 2 -l 2 ../models/Eratosthenes-PT-010/model.pnml ../models/Eratosthenes-PT-010/ReachabilityCardinality.xml > logFiles/Eratosthenes_s.txt

#echo "DatabaseWithMutex" | ts
#time ./verifypn-linux64 -x 2 -l 2 ../models/DatabaseWithMutex-PT-02/model.pnml ../models/DatabaseWithMutex-PT-02/ReachabilityCardinality.xml > logFiles/DatabaseWithMutex_s.txt



#echo "CircularTrains-PT-048" | ts
#time ./verifypn-linux64 -x 1 -l 1 -s OverApprox ../models/CircularTrains-PT-048/model.pnml ../models/CircularTrains-PT-048/ReachabilityCardinality.xml


#echo "CircularTrains-PT-096" | ts
#time ./verifypn-linux64 -x 1 -l 2 -s OverApprox ../models/CircularTrains-PT-096/model.pnml ../models/CircularTrains-PT-096/ReachabilityCardinality.xml > logFiles/CircularTrains-PT-096_LTS.txt


#echo "CircularTrains-PT-192" | ts
#time ./verifypn-linux64 -x 1 -l 2 -s OverApprox ../models/CircularTrains-PT-192/model.pnml ../models/CircularTrains-PT-192/ReachabilityCardinality.xml > logFiles/CircularTrains-PT-192_LTS.txt


#echo "CircularTrains-PT-384" | ts
#time ./verifypn-linux64 -x 1 -l 2 -s OverApprox ../models/CircularTrains-PT-384/model.pnml ../models/CircularTrains-PT-384/ReachabilityCardinality.xml > logFiles/CircularTrains-PT-384_LTS.txt


#echo "CircularTrains-PT-768" | ts
#time ./verifypn-linux64 -x 1 -l 2 -s OverApprox ../models/CircularTrains-PT-768/model.pnml ../models/CircularTrains-PT-768/ReachabilityCardinality.xml > logFiles/CircularTrains-PT-768_LTS.txt