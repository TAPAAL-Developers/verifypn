#!/bin/bash

inputfile=$1
comparefile=comparelog.log
outputfile=correctlog.log
marciefile=marcie.log

#Parse the results from Marcie
bash parseMarcieres.sh | tee $marciefile


#Compare these results to our results
#do_stuff >> $comparefile



cat $comparefile | grep NOT_CORRECT >> $outputfile
