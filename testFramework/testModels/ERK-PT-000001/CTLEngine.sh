PROGRAMPATH=$1
ENGINE=$2
MODELFILE=$3
QUERYFILE=$4

CURRENTDIR="`basename "$PWD"`"

FILENAME=$(basename "$QUERYFILE")
FILENAME="${FILENAME%%.*}"

LOGFILE="$CURRENTDIR-$FILENAME.log"

echo "Running CTLEngine"
$1 $2 $3 $4 
