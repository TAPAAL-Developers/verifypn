#!/bin/bash
echo "Algorithm, Search strategy, query, time(ms, #configurations, #markings, #cycles, #evil" >> result_q.csv

for R in *.log; do

    awk '/Algorithm:/{print $NF}' $R >> meta.log
    sed -n '1p' meta.log >> tmp.log
    rm meta.log
    mv tmp.log meta.log

    awk '/Search:/{print $NF}' $R >> meta.log
    sed -n '1,2p' meta.log >> tmp.log
    rm meta.log
    mv tmp.log meta.log

    awk '/FORMULA/{print $2, $3}' $R >> meta.log

    awk '/Search elapsed time for query/{print $8}' $R >> meta.log

    awk '/Configurations:/{print $3}' $R >> meta.log

    awk '/Markings:/{print $NF}' $R >>  meta.log

    awk '/Detected:/{print $3}' $R >> meta.log

    awk '/Evil Cycles:/{print $6}' $R >> meta.log

    LINE=$(wc -l < meta.log)

    

    eval I=$[($LINE-2)/6]

    while [ $I -gt 0 ]; do
        #LINE=$[$LINE-2]
        count=$[2+$I]

        

        for P in meta.log; do
        {
            read engine
            read search
            q=$(sed -n ' '$count' 'p meta.log)
            count=$[$count + ( ($LINE-2) /6)]
                
            tim=$(sed -n ' '$count' 'p meta.log)
            count=$[$count + ( ($LINE-2) /6)]
                
            conf=$(sed -n ' '$count' 'p meta.log)
            count=$[$count + ( ($LINE-2) /6)]
                
            mark=$(sed -n ' '$count' 'p meta.log)
            count=$[$count + ( ($LINE-2) /6)]   
            
            cycle=$(sed -n ' '$count' 'p meta.log)
            count=$[$count + ( ($LINE-2) /6)]  
            
            evil=$(sed -n ' '$count' 'p meta.log)
            
            echo "$engine, $search, $q, $tim, $conf, $mark, $cycle, $evil" >> result_q.csv
        } < $P
        done
    
        I=$[$I-1]
    done
    

    rm meta.log
done