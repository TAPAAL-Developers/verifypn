#!/bin/bash
./verifypn-linux64 -ctl testFramework/ModelDB/HouseConstruction-PT-002/model.pnml testFramework/ModelDB/HouseConstruction-PT-002/CTLFireabilitySimple.xml >> log.log
./verifypn-linux64 -ctl testFramework/ModelDB/HouseConstruction-PT-002/model.pnml testFramework/ModelDB/HouseConstruction-PT-002/CTLFireabilitySimple.xml -czero >> log.log
./verifypn-linux64 -ctl testFramework/ModelDB/HouseConstruction-PT-002/model.pnml testFramework/ModelDB/HouseConstruction-PT-002/CTLFireability.xml >> log.log
./verifypn-linux64 -ctl testFramework/ModelDB/HouseConstruction-PT-002/model.pnml testFramework/ModelDB/HouseConstruction-PT-002/CTLFireability.xml -czero >> log.log

./verifypn-linux64 -ctl testFramework/ModelDB/TokenRing-PT-005/model.pnml testFramework/ModelDB/TokenRing-PT-005/CTLFireabilitySimple.xml >> log.log
./verifypn-linux64 -ctl testFramework/ModelDB/TokenRing-PT-005/model.pnml testFramework/ModelDB/TokenRing-PT-005/CTLFireabilitySimple.xml -czero >> log.log
./verifypn-linux64 -ctl testFramework/ModelDB/TokenRing-PT-005/model.pnml testFramework/ModelDB/TokenRing-PT-005/CTLFireability.xml >> log.log
./verifypn-linux64 -ctl testFramework/ModelDB/TokenRing-PT-005/model.pnml testFramework/ModelDB/TokenRing-PT-005/CTLFireability.xml -czero >> log.log

./verifypn-linux64 -ctl testFramework/ModelDB/SimpleLoadBal-PT-02/model.pnml testFramework/ModelDB/SimpleLoadBal-PT-02/CTLFireabilitySimple.xml >> log.log
./verifypn-linux64 -ctl testFramework/ModelDB/SimpleLoadBal-PT-02/model.pnml testFramework/ModelDB/SimpleLoadBal-PT-02/CTLFireabilitySimple.xml -czero >> log.log
./verifypn-linux64 -ctl testFramework/ModelDB/SimpleLoadBal-PT-02/model.pnml testFramework/ModelDB/SimpleLoadBal-PT-02/CTLFireability.xml >> log.log
./verifypn-linux64 -ctl testFramework/ModelDB/SimpleLoadBal-PT-02/model.pnml testFramework/ModelDB/SimpleLoadBal-PT-02/CTLFireability.xml -czero >> log.log

./verifypn-linux64 -ctl testFramework/ModelDB/ERK-PT-000001/model.pnml testFramework/ModelDB/ERK-PT-000001/CTLFireabilitySimple.xml >> log.log
./verifypn-linux64 -ctl testFramework/ModelDB/ERK-PT-000001/model.pnml testFramework/ModelDB/ERK-PT-000001/CTLFireabilitySimple.xml -czero >> log.log
./verifypn-linux64 -ctl testFramework/ModelDB/ERK-PT-000001/model.pnml testFramework/ModelDB/ERK-PT-000001/CTLFireability.xml >> log.log
./verifypn-linux64 -ctl testFramework/ModelDB/ERK-PT-000001/model.pnml testFramework/ModelDB/ERK-PT-000001/CTLFireability.xml -czero >> log.log

./verifypn-linux64 -ctl testFramework/ModelDB/Angiogenesis-PT-01/model.pnml testFramework/ModelDB/Angiogenesis-PT-01/CTLFireabilitySimple.xml >> log.log
./verifypn-linux64 -ctl testFramework/ModelDB/Angiogenesis-PT-01/model.pnml testFramework/ModelDB/Angiogenesis-PT-01/CTLFireabilitySimple.xml -czero >> log.log
./verifypn-linux64 -ctl testFramework/ModelDB/Angiogenesis-PT-01/model.pnml testFramework/ModelDB/Angiogenesis-PT-01/CTLFireability.xml >> log.log
./verifypn-linux64 -ctl testFramework/ModelDB/Angiogenesis-PT-01/model.pnml testFramework/ModelDB/Angiogenesis-PT-01/CTLFireability.xml -czero >> log.log

cat log.log | grep FORMULA >> a.log
rm log.log
