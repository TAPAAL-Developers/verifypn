#!/bin/bash
echo "CircularTrains-PT-012" | ts
time ./verifypn-linux64 -x 1 -l 2 -s OverApprox ../models/CircularTrains-PT-012/model.pnml ../models/CircularTrains-PT-012/ReachabilityCardinality.xml > logFiles/CircularTrains-PT-012_LTS.txt


echo "CircularTrains-PT-024" | ts
time ./verifypn-linux64 -x 1 -l 2 -s OverApprox ../models/CircularTrains-PT-024/model.pnml ../models/CircularTrains-PT-024/ReachabilityCardinality.xml > logFiles/CircularTrains-PT-024_LTS.txt


echo "CircularTrains-PT-048" | ts
time ./verifypn-linux64 -x 1 -l 2 -s OverApprox ../models/CircularTrains-PT-048/model.pnml ../models/CircularTrains-PT-048/ReachabilityCardinality.xml > logFiles/CircularTrains-PT-048_LTS.txt


echo "CircularTrains-PT-096" | ts
time ./verifypn-linux64 -x 1 -l 2 -s OverApprox ../models/CircularTrains-PT-096/model.pnml ../models/CircularTrains-PT-096/ReachabilityCardinality.xml > logFiles/CircularTrains-PT-096_LTS.txt


echo "CircularTrains-PT-192" | ts
time ./verifypn-linux64 -x 1 -l 2 -s OverApprox ../models/CircularTrains-PT-192/model.pnml ../models/CircularTrains-PT-192/ReachabilityCardinality.xml > logFiles/CircularTrains-PT-192_LTS.txt


echo "CircularTrains-PT-384" | ts
time ./verifypn-linux64 -x 1 -l 2 -s OverApprox ../models/CircularTrains-PT-384/model.pnml ../models/CircularTrains-PT-384/ReachabilityCardinality.xml > logFiles/CircularTrains-PT-384_LTS.txt


echo "CircularTrains-PT-768" | ts
time ./verifypn-linux64 -x 1 -l 2 -s OverApprox ../models/CircularTrains-PT-768/model.pnml ../models/CircularTrains-PT-768/ReachabilityCardinality.xml > logFiles/CircularTrains-PT-768_LTS.txt