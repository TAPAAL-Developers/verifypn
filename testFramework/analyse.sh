echo "========================================================="
echo "========================================================="
echo "============== TEST FRAMEWORK ANALYSING ================="
echo "========================================================="
echo "========================================================="
#grep prints line - p1:search pattern - p2: input file
RESULTFILE=$1
cat $RESULTFILE | grep ERROR | awk '{print $1,$2,$3}' |\
         sort  | uniq | awk '{print $2}' | uniq -d

cat $RESULTFILE | grep STATE_SPACE  |\
         sort | uniq | awk '{print $2}' | uniq -d
#| awk '{print $1,$2,$3}' | 
echo "     >>>>>>>>>>>>>Under construction<<<<<<<<<<<<<<<"
echo "========================================================="
