#!/bin/bash

OUTPUT="$mname-$qname-$aname-$sname.log"
ENGINE="-ctl"
TIMEOUT=60

if [[ -f "$OUTPUT" ]]; then
	rm "$OUTPUT"
fi

case $aname in
	"local" )
                echo "Algorithm: $aname" >> $mname-$qname-$aname-$sname.log;
		echo "Search: $sname" >> "$OUTPUT"
		echo "Query type: $qname"  >> "$OUTPUT"
		{ timeout $TIMEOUT "$1" "$2" "$3" "$4" "local" "-s" "$sname" "-x" "$5"; } >> "$OUTPUT" 2>&1
		;;
	"local-i" )
                echo "Algorithm: $aname" >> $mname-$qname-$aname-$sname.log;
                echo "Search: $sname" >> "$OUTPUT"
                echo "Query type: $qname"  >> "$OUTPUT"
                { timeout $TIMEOUT "$1" "$2" "$3" "$4" "local-i" "-s" "$sname" "-r 1" "-x" "$5"; } >> "$OUTPUT" 2>&1
                ;;
	"global" )
                echo "Algorithm: $aname" >> $mname-$qname-$aname-$sname.log;
		echo "Search: $sname" >> "$OUTPUT"
		echo "Query type: $qname"  >> "$OUTPUT"
		{ timeout $TIMEOUT "$1" "$2" "$3" "$4" "global" "-s" "$sname" "-x" "$5"; } >> "$OUTPUT" 2>&1
		;;
	"global-i" )
                echo "Algorithm: $aname" >> $mname-$qname-$aname-$sname.log;
                echo "Search: $sname" >> "$OUTPUT"
                echo "Query type: $qname"  >> "$OUTPUT"
                { timeout $TIMEOUT "$1" "$2" "$3" "$4" "global-i" "-s" "$sname" "-r 1" "-x" "$5"; } >> "$OUTPUT" 2>&1
                ;;
	"czero" )
                echo "Algorithm: $aname" >> $mname-$qname-$aname-$sname.log;
		echo "Search: $sname" >> "$OUTPUT"
		echo "Query type: $qname"  >> "$OUTPUT"
		{ timeout $TIMEOUT "$1" "$2" "$3" "$4" "czero" "-s" "$sname" "-x" "$5"; }  >> "$OUTPUT" 2>&1
		;;
	"czero-i" )
                echo "Algorithm: $aname" >> $mname-$qname-$aname-$sname.log;
                echo "Search: $sname" >> "$OUTPUT"
                echo "Query type: $qname"  >> "$OUTPUT"
                { timeout $TIMEOUT "$1" "$2" "$3" "$4" "czero-i" "-s" "$sname" "-r 1" "-x" "$5"; }  >> "$OUTPUT" 2>&1
                ;;
esac
