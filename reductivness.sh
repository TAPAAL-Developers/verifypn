#!/bin/bash
#echo "CircularTrains-PT-012" | ts
#time ./verifypn-linux64 -x 1 -l 1 -s OverApprox ../models/CircularTrains-PT-012/model.pnml ../models/CircularTrains-PT-012/ReachabilityCardinality.xml

for modelname in  CircadianClock-PT-000001 CircularTrains-PT-768 CSRepetitions-PT-02 DatabaseWithMutex-PT-02 Dekker-PT-010 Diffusion2D-PT-D05N010 DrinkVendingMachine-PT-02 Echo-PT-d02r09 EnergyBus-PT-none Eratosthenes-PT-010 ERK-PT-000001 FMS-PT-002 GlobalResAllocation-PT-03 HouseConstruction-PT-002 IBMB2S565S3960-PT-none Kanban-PT-0005 LamportFastMutEx-PT-2 MAPK-PT-008 MultiwaySync-PT-none NeoElection-PT-2 ParamProductionCell-PT-0 PermAdmissibility-PT-01 Peterson-PT-2 PhilosophersDyn-PT-03 Philosophers-PT-000005 Planning-PT-none PolyORBLF-PT-S02J04T06 PolyORBNT-PT-S05J20 ProductionCell-PT-none QuasiCertifProtocol-PT-02 Railroad-PT-005 ResAllocation-PT-R002C002 Ring-PT-none RwMutex-PT-r0010w0010 SharedMemory-PT-000005 SimpleLoadBal-PT-02 Solitaire-PT-EngCT7x7 TokenRing-PT-005 UtahNoC-PT-none Vasy2003-PT-none

do
    for query in ReachabilityCardinality ReachabilityFireability ReachabilityFireabilitySimple ReachabilityDeadlock
    do 
        ./verifypn-linux64 -o mc -r 2 -b ../models/$modelname/model.pnml ../models/$modelname/$query.xml > logFiles/$modelname$query.txt
    done
done
#Angiogenesis-PT-01 ARMCacheCoherence-PT-none