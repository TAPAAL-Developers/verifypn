../verifypn-linux64 -ctl testFramework/ModelDB/CircularTrains-PT-012/model.pnml ModelDB/CircularTrains-PT-012/CTLCardinality.xml >> log.log
#../verifypn-linux64 -ctl ModelDB/CircularTrains-PT-012/model.pnml ModelDB/CircularTrains-PT-012/CTLCardinality.xml >> log.log
rm log.log
#cat log.log | grep FORMULA >> a.log

