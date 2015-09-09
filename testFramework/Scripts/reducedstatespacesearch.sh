#!/bin/bash

INPUTSPATH=/home/mossns/Documents/launchpad/INPUTS

for D in $(find ${INPUTSPATH} -mindepth 1 -maxdepth 1 -type d) ; do
    echo "----------------------------------------------------------------------" >> actualStateSpace.log
    echo "--------------------------------NEW MODEL-----------------------------" >> actualStateSpace.log
    echo "----------------------------------------------------------------------" >> actualStateSpace.log
    echo "$D" >> actualStateSpace.log
    ./verifypn-linux64 $D/model.pnml -e -n -o mc -b -c 6 >> actualStateSpace.log
done
