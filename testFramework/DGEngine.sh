#!/bin/bash

OUTPUT="$mname-$qname-$aname-$sname.log"
ENGINE="-ctl"
TIMEOUT=3600

if [[ -f "$OUTPUT" ]]; then
	rm "$OUTPUT"
fi

case $aname in
	"local" )
                echo "Algorithm: $aname" >> $mname-$qname-$aname-$sname.log;
		echo "Search: $sname" >> "$OUTPUT"
		echo "Query type: $qname"  >> "$OUTPUT"
		{ timeout $TIMEOUT "$1" "$2" "$3" "$4" "local" "-s" "$sname"; } >> "$OUTPUT" 2>&1
		;;
	"global" )
                echo "Algorithm: $aname" >> $mname-$qname-$aname-$sname.log;
		echo "Search: $sname" >> "$OUTPUT"
		echo "Query type: $qname"  >> "$OUTPUT"
		{ timeout $TIMEOUT "$1" "$2" "$3" "$4" "global" "-s" "$sname"; } >> "$OUTPUT" 2>&1
		;;
	"czero" )
                echo "Algorithm: $aname" >> $mname-$qname-$aname-$sname.log;
		echo "Search: $sname" >> "$OUTPUT"
		echo "Query type: $qname"  >> "$OUTPUT"
		{ timeout $TIMEOUT "$1" "$2" "$3" "$4" "czero" "-s" "$sname"; }  >> "$OUTPUT" 2>&1
		;;
esac