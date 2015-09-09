echo "=================== -O 1 ============================"
./runLTSmin.sh dfs 4 false -mc -O1 >> optitest.log
echo "=================== -O 2 ============================"
./runLTSmin.sh dfs 4 false -mc -O2 >> optitest.log
echo "=================== -o 3 ============================"
./runLTSmin.sh dfs 4 false -mc -O3 >> optitest.log
