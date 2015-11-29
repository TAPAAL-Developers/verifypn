#!/bin/bash

OUTPUT="$mname-$qname-$aname-$sname.log"
ENGINE="-ctl"
TIMEOUT=10

if [[ -f "$OUTPUT" ]]; then
	rm "$OUTPUT"
fi

case $aname in
	"local" )
		echo "Search: local $sname" >> "$OUTPUT"
		echo "Query type: $qname"  >> "$OUTPUT"
		{ timeout $TIMEOUT time "$1" "$2" "$3" "$4" "local" "-s" "$sname"; } >> "$OUTPUT" 2>&1
		;;
	"global" )
		echo "Search: global $sname" >> "$OUTPUT"
		echo "Query type: $qname"  >> "$OUTPUT"
		{ timeout $TIMEOUT time "$1" "$2" "$3" "$4" "global" "-s" "$sname"; } >> "$OUTPUT" 2>&1
		;;
	"czero" )
		echo "Search: czero $sname" >> "$OUTPUT"
		echo "Query type: $qname"  >> "$OUTPUT"
		{ timeout $TIMEOUT time "$1" "$2" "$3" "$4" "czero" "-s" "$sname"; }  >> "$OUTPUT" 2>&1
		;;
esac