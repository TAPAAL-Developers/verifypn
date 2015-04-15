#!/bin/bash
echo "----------------------------------------------------------------------------------------------------------------------------"
echo "-----------------------------------------------Testing Reduction effect-----------------------------------------------------"
echo "----------------------------------------------------------------------------------------------------------------------------"
#Small
#echo "TokenRing" | ts
#time ./verifypn-linux64 -x 1 -s LTSmin ../models/TokenRing/PT/TokenRing-5-unfolded.pnml ../models/HouseConstruction-PT-002/ReachabilityDeadlock.xml

#echo "SimpleLoadBal" | ts
#time ./verifypn-linux64 -x 1 -s LTSmin ../models/SimpleLoadBal/PT/simple_lbs-2.pnml ../models/HouseConstruction-PT-002/ReachabilityDeadlock.xml

echo "----------------------------------------------------------------------------------------------------------------------------"
echo "-----------------------------------------------Concartinated Queries-----------------------------------------------------"
echo "----------------------------------------------------------------------------------------------------------------------------"
#echo "Philosophers" | ts
#time ./verifypn-linux64 -x 1 -l 2 -s OverApprox -r 2 ../models/Philosophers-PT-000005/model.pnml ../models/Philosophers-PT-000005/ReachabilityCardinality.xml > logFiles/Philosophers.txt

#echo "ERK" | ts
#time ./verifypn-linux64 -x 1 -l 2 -s OverApprox -r 2 ../models/ERK-PT-000001/model.pnml ../models/ERK-PT-000001/ReachabilityCardinality.xml > logFiles/ERK.txt

#echo "HouseConstruction Small" | ts
#time ./verifypn-linux64 -x 1 -l 2 -s OverApprox -r 2 ../models/HouseConstruction-PT-002/model.pnml ../models/HouseConstruction-PT-002/ReachabilityCardinality.xml > logFiles/HouseConstructionSmall.txt

#echo "HouseConstruction Big" | ts
#time ./verifypn-linux64 -x 1 -l 2 -s OverApprox -r 2 ../models/HouseConstruction/PT/HouseConstruction-500.pnml ../models/HouseConstruction-PT-002/ReachabilityCardinality.xml > logFiles/HouseConstructionBig.txt

echo "FMS" | ts
time ./verifypn-linux64 -x 1 -l 2 -d -r 2 ../models/FMS-PT-002/model.pnml ../models/FMS-PT-002/ReachabilityCardinality.xml > logFiles/FMS.txt

echo "MAPK" | ts
time ./verifypn-linux64 -x 1 -l 2 -s OverApprox -r 2 ../models/MAPK-PT-008/model.pnml ../models/MAPK-PT-008/ReachabilityCardinality.xml > logFiles/MAPK.txt

#echo "Echo" | ts "THIS IS TOO BIG! "
#time ./verifypn-linux64 -x 1 -l 2 -d -r 2 ../models/Echo-PT-d02r09/model.pnml ../models/Echo-PT-d02r09/ReachabilityCardinality.xml > logFiles/Echo.txt

echo "Kanban" | ts
time ./verifypn-linux64 -x 1 -l 2 -d -r 2 ../models/Kanban-PT-0005/model.pnml ../models/Kanban-PT-0005/ReachabilityCardinality.xml > logFiles/Kanban.txt

echo "----------------------------------------------------------------------------------------------------------------------------"
echo "-----------------------------------------------------Single Queries----------------------------------------------------------"
echo "----------------------------------------------------------------------------------------------------------------------------"

#echo "Philosophers" | ts
#time ./verifypn-linux64 -x 2 -l 1 -s OverApprox -r 2 ../models/Philosophers-PT-000005/model.pnml ../models/Philosophers-PT-000005/ReachabilityCardinality.xml > logFiles/Philosophers_s.txt

#echo "ERK" | ts
#time ./verifypn-linux64 -x 2 -l 1 -s OverApprox -r 2 ../models/ERK-PT-000001/model.pnml ../models/ERK-PT-000001/ReachabilityCardinality.xml > logFiles/ERK_s.txt

#echo "HouseConstruction Small" | ts
#time ./verifypn-linux64 -x 2 -l 1 -s OverApprox -r 2 ../models/HouseConstruction-PT-002/model.pnml ../models/HouseConstruction-PT-002/ReachabilityCardinality.xml > logFiles/HouseConstructionSmall_s.txt

#echo "HouseConstruction Big" | ts
#time ./verifypn-linux64 -x 2 -l 1 -s OverApprox -r 2 ../models/HouseConstruction/PT/HouseConstruction-500.pnml ../models/HouseConstruction-PT-002/ReachabilityCardinality.xml > logFiles/HouseConstructionBig_s.txt

#echo "Diffusion2D" | ts
#time ./verifypn-linux64 -x 2 -l 1 -s OverApprox -r 2 ../models/Diffusion2D-PT-D05N010/model.pnml ../models/Diffusion2D-PT-D05N010/ReachabilityCardinality.xml > logFiles/Diffusion2D_s.txt

echo "FMS" | ts
time ./verifypn-linux64 -x 2 -l 1 -d -r 2 ../models/FMS-PT-002/model.pnml ../models/FMS-PT-002/ReachabilityCardinality.xml > logFiles/FMS_s.txt

echo "MAPK" | ts
time ./verifypn-linux64 -x 2 -l 1 -s OverApprox -r 2 ../models/MAPK-PT-008/model.pnml ../models/MAPK-PT-008/ReachabilityCardinality.xml > logFiles/MAPK_s.txt

#echo "Echo" | ts
#time ./verifypn-linux64 -x 2 -l 1 -d -r 2 ../models/Echo-PT-d02r09/model.pnml ../models/Echo-PT-d02r09/ReachabilityCardinality.xml > logFiles/Echo_s.txt

echo "Kanban" | ts
time ./verifypn-linux64 -x 2 -l 1 -d -r 2 ../models/Kanban-PT-0005/model.pnml ../models/Kanban-PT-0005/ReachabilityCardinality.xml > logFiles/Kanban_s.txt


#NeoElection-PT-2
#EnergyBus-PT-none
#Angiogenesis-PT-01
#CSRepetitions-PT-02
#Eratosthenes-PT-010
#MultiwaySync-PT-none
#CircularTrains-PT-012
#LamportFastMutEx-PT-2

#DatabaseWithMutex-PT-02
#PermAdmissibility-PT-01
#CircadianClock-PT-000001
#ParamProductionCell-PT-0
#DrinkVendingMachine-PT-02
#GlobalResAllocation-PT-03



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