#!/bin/bash
echo "----------------------------------------------------------------------------------------------------------------------------"
echo "-----------------------------------------------Testing Reduction effect-----------------------------------------------------"
echo "----------------------------------------------------------------------------------------------------------------------------"
#Small
#echo "TokenRing" | ts
#time ./verifypn-linux64 -x 1 -s LTSmin ../models/TokenRing/PT/TokenRing-5-unfolded.pnml ../models/HouseConstruction-PT-002/ReachabilityDeadlock.xml

#echo "SimpleLoadBal" | ts
#time ./verifypn-linux64 -x 1 -s LTSmin ../models/SimpleLoadBal/PT/simple_lbs-2.pnml ../models/HouseConstruction-PT-002/ReachabilityDeadlock.xml

echo "Philosophers" | ts
./verifypn-linux64 -x 1 -l 2 -s OverApprox -r 2 ../models/Philosophers-PT-000005/model.pnml ../models/Philosophers-PT-000005/ReachabilityCardinality.xml > Philosophers.txt

echo "ERK" | ts
./verifypn-linux64 -x 1 -l 2 -s OverApprox -r 2 ../models/ERK-PT-000001/model.pnml ../models/ERK-PT-000001/ReachabilityCardinality.xml > ERK.txt

echo "HouseConstruction Small" | ts
./verifypn-linux64 -x 1 -l 2 -s OverApprox -r 2 ../models/HouseConstruction-PT-002/model.pnml ../models/HouseConstruction-PT-002/ReachabilityCardinality.xml > HouseConstructionSmall.txt

echo "HouseConstruction Big" | ts
./verifypn-linux64 -x 1 -l 2 -s OverApprox -r 2 ../models/HouseConstruction/PT/HouseConstruction-500.pnml ../models/HouseConstruction-PT-002/ReachabilityCardinality.xml > HouseConstructionBig.txt



#echo "IBMB2S565S3960" | ts
#time ./verifypn-linux64 -x 1 -s LTSmin ../models/IBMB2S565S3960-PT-none/model.pnml ../models/IBMB2S565S3960-PT-none/ReachabilityDeadlock.xml
#sh runLTS.sh

#echo "QuasiCertifProtocol" | ts
#time ./verifypn-linux64 -x 1 -s LTSmin ../models/QuasiCertifProtocol/PT/QCertifProtocol_32-unfold.pnml ../models/ParamProductionCell-PT-0/ReachabilityDeadlock.xml
#sh runLTS.sh

#Big
#echo "Diffusion2D" | ts
#time ./verifypn-linux64 -x 1 ../models/Diffusion2D/PT/2D8_gradient_50x50_150.pnml ../models/Diffusion2D-PT-D05N010/ReachabilityDeadlock.xml
#echo "CircularTrains" | ts
#time ./verifypn-linux64 -x 1 ../models/CircularTrains/PT/CircularTrain-768.pnml ../models/CircularTrains-PT-012/ReachabilityDeadlock.xml
#echo "HouseConstruction" | ts
#time ./verifypn-linux64 -x 1 ../models/HouseConstruction/PT/HouseConstruction-500.pnml ../models/HouseConstruction-PT-002/ReachabilityDeadlock.xml
#echo "ParamProductionCell" | ts
#time ./verifypn-linux64 -x 1 ../models/ParamProductionCell/PT/open_system_0.pnml ../models/ParamProductionCell-PT-0/ReachabilityDeadlock.xml
#echo "ResAllocation" | ts
#time ./verifypn-linux64 -x 1 ../models/ResAllocation/PT/RAS-R-100.pnml ../models/HouseConstruction-PT-002/ReachabilityDeadlock.xml