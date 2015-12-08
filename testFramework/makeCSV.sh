
#!/bin/bash
echo "Algorithm, Search strategy, Query, Modelname, #Results)" >> result.csv

for R in *.log; do
    awk '/Algorithm:/{print $NF}' $R >> meta.log
    sed -n '1p' meta.log >> tmp.log
    rm meta.log
    mv tmp.log meta.log

    awk '/Search:/{print $NF}' $R >> meta.log
        sed -n '1,2p' meta.log >> tmp.log
    rm meta.log
    mv tmp.log meta.log

    awk '/Query type:/{print $NF}' $R >> meta.log
    sed -n '1,3p' meta.log >> tmp.log
    rm meta.log
    mv tmp.log meta.log

    awk '/Modelname:/{print $NF}' $R >> meta.log
    sed -n '1,4p' meta.log >> tmp.log
    rm meta.log
    mv tmp.log meta.log

     awk '/FORMULA/ {count++} END{print count}' $R >> meta.log



    for P in meta.log; do
    {
        read engine
                read search
        read q
        read name
        read formula
        echo "$engine, $search, $q, $name, $formula" >> result.csv
    } < $P
    done
    rm meta.log
done
