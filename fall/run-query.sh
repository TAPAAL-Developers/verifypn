MODEL_FILE=$1
QUERY_FILE=$2
QUERY_NUMBER=$3
BINARY=$4
ALG=$5
OUTPUT_FILE=$6

MPI=""

# Only use a call to MPI if we
# use the distributed algorithm
export MAXMEM_KB=15000000
if [ "$ALG" == "dist" ] 
then
    MPI_CALL="mpirun -np 64"
fi

# <---- Print settings for debugging and verification ----> #
echo "Binary: $BINARY"
echo "Alg: $ALG MPI: $MPI_CALL"
echo "Model: $MODEL_FILE Query: $QUERY_FILE Query number: $QUERY_NUMBER" 
echo "Timeout: $TIMEOUT Memory limit (kb): $MAXMEM_KB" 

# <---- Handle the calls to the binary ----> #
if [ "$ALG" == "dist" ]
then
    "$MPI_CALL" "$BINARY" "$MODEL_FILE" "$QUERY_FILE" -ctl "$ALG" -s DFS -x "$QUERY_NUMBER" > "$OUTPUT_FILE" 2>&1;
fi

if [ "$ALG" != "dist" ]
then
    "$BINARY" "$MODEL_FILE" "$QUERY_FILE" -ctl "$ALG" -s DFS -x "$QUERY_NUMBER" > "$OUTPUT_FILE" 2>&1;
fi

# <---- Print result for debugging and verification ----> #
cat "$OUTPUT_FILE"
