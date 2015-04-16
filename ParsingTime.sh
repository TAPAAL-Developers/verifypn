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

#echo "FMS" | ts
#time ./verifypn-linux64 -x 1 -l 2 -d -r 2 ../models/FMS-PT-002/model.pnml ../models/FMS-PT-002/ReachabilityCardinality.xml > logFiles/FMS.txt

#echo "MAPK" | ts
#time ./verifypn-linux64 -x 1 -l 2 -s OverApprox -r 2 ../models/MAPK-PT-008/model.pnml ../models/MAPK-PT-008/ReachabilityCardinality.xml > logFiles/MAPK.txt

#echo "Echo" | ts 
#time ./verifypn-linux64 -x 1 -l 2 -d -r 2 ../models/Echo-PT-d02r09/model.pnml ../models/Echo-PT-d02r09/ReachabilityCardinality.xml > logFiles/Echo.txt

#echo "Kanban" | ts
#time ./verifypn-linux64 -x 1 -l 2 -d -r 2 ../models/Kanban-PT-0005/model.pnml ../models/Kanban-PT-0005/ReachabilityCardinality.xml > logFiles/Kanban.txt

#echo "MultiwaySync" | ts
#time ./verifypn-linux64 -x 1 -l 2 -d -r 2 ../models/MultiwaySync-PT-none/model.pnml ../models/MultiwaySync-PT-none/ReachabilityCardinality.xml > logFiles/Multiway.txt

#echo "NeoElection" | ts
#time ./verifypn-linux64 -x 1 -l 2 -d -r 2 ../models/NeoElection-PT-2/model.pnml ../models/NeoElection-PT-2/ReachabilityCardinality.xml > logFiles/NeoElection.txt

echo "EnergyBus" | ts
time ./verifypn-linux64 -x 1 -l 2 -d -r 2 ../models/EnergyBus-PT-none/model.pnml ../models/EnergyBus-PT-none/ReachabilityCardinality.xml > logFiles/EnergyBus.txt

echo "Angiogenesis" | ts
time ./verifypn-linux64 -x 1 -l 2 -d -r 2 ../models/Angiogenesis-PT-01/model.pnml ../models/Angiogenesis-PT-01/ReachabilityCardinality.xml > logFiles/Angiogenesis.txt

echo "CSRepetitions" | ts
time ./verifypn-linux64 -x 1 -l 2 -d -r 2 ../models/CSRepetitions-PT-02/model.pnml ../models/CSRepetitions-PT-02/ReachabilityCardinality.xml > logFiles/CSRepetitions.txt

echo "Eratosthenes" | ts
time ./verifypn-linux64 -x 1 -l 2 -d -r 2 ../models/Eratosthenes-PT-010/model.pnml ../models/Eratosthenes-PT-010/ReachabilityCardinality.xml > logFiles/Eratosthenes.txt

echo "DatabaseWithMutex" | ts
time ./verifypn-linux64 -x 1 -l 2 -d -r 2 ../models/DatabaseWithMutex-PT-02/model.pnml ../models/DatabaseWithMutex-PT-02/ReachabilityCardinality.xml > logFiles/DatabaseWithMutex.txt

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

#echo "FMS" | ts
#time ./verifypn-linux64 -x 2 -l 1 -d -r 2 ../models/FMS-PT-002/model.pnml ../models/FMS-PT-002/ReachabilityCardinality.xml > logFiles/FMS_s.txt

#echo "MAPK" | ts
#time ./verifypn-linux64 -x 2 -l 1 -s OverApprox -r 2 ../models/MAPK-PT-008/model.pnml ../models/MAPK-PT-008/ReachabilityCardinality.xml > logFiles/MAPK_s.txt

#echo "Echo" | ts
#time ./verifypn-linux64 -x 2 -l 1 -d -r 2 ../models/Echo-PT-d02r09/model.pnml ../models/Echo-PT-d02r09/ReachabilityCardinality.xml > logFiles/Echo_s.txt

#echo "Kanban" | ts
#time ./verifypn-linux64 -x 2 -l 1 -d -r 2 ../models/Kanban-PT-0005/model.pnml ../models/Kanban-PT-0005/ReachabilityCardinality.xml > logFiles/Kanban_s.txt

#echo "MultiwaySync" | ts
#time ./verifypn-linux64 -x 2 -l 1 -d -r 2 ../models/MultiwaySync-PT-none/model.pnml ../models/MultiwaySync-PT-none/ReachabilityCardinality.xml > logFiles/Multiway_s.txt

#echo "NeoElection" | ts
#time ./verifypn-linux64 -x 2 -l 1 -d -r 2 ../models/NeoElection-PT-2/model.pnml ../models/NeoElection-PT-2/ReachabilityCardinality.xml > logFiles/NeoElection_s.txt

echo "EnergyBus" | ts
time ./verifypn-linux64 -x 2 -l 1 -d -r 2 ../models/EnergyBus-PT-none/model.pnml ../models/EnergyBus-PT-none/ReachabilityCardinality.xml > logFiles/EnergyBus_s.txt

echo "Angiogenesis" | ts
time ./verifypn-linux64 -x 2 -l 1 -d -r 2 ../models/Angiogenesis-PT-01/model.pnml ../models/Angiogenesis-PT-01/ReachabilityCardinality.xml > logFiles/Angiogenesis_s.txt

echo "CSRepetitions" | ts
time ./verifypn-linux64 -x 2 -l 1 -d -r 2 ../models/CSRepetitions-PT-02/model.pnml ../models/CSRepetitions-PT-02/ReachabilityCardinality.xml > logFiles/CSRepetitions_s.txt

echo "Eratosthenes" | ts
time ./verifypn-linux64 -x 2 -l 1 -d -r 2 ../models/Eratosthenes-PT-010/model.pnml ../models/Eratosthenes-PT-010/ReachabilityCardinality.xml > logFiles/Eratosthenes_s.txt

echo "DatabaseWithMutex" | ts
time ./verifypn-linux64 -x 2 -l 1 -d -r 2 ../models/DatabaseWithMutex-PT-02/model.pnml ../models/DatabaseWithMutex-PT-02/ReachabilityCardinality.xml > logFiles/DatabaseWithMutex_s.txt


#

#LamportFastMutEx-PT-2
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