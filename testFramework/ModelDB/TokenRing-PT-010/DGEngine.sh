#!/bin/bash

OUTPUT="$mname-$qname-$sname.log"
ENGINE="-ctl"

if [[ -f "$OUTPUT" ]]; then
	rm "$OUTPUT"
fi

case $sname in
	"local" )
		echo "Search: local" >> "$OUTPUT"
		echo "Query type: $qname"  >> "$OUTPUT"
		{ time "$1" "$2" "$3" "$4"; } >> "$OUTPUT" 2>&1
		;;
	"global" )
		echo "Search: global" >> "$OUTPUT"
		echo "Query type: $qname"  >> "$OUTPUT"
		{ time "$1" "$2" "$3" "$4" "-global"; } >> "$OUTPUT" 2>&1
		;;
	"czero" )
		echo "Search: czero" >> "$OUTPUT"
		echo "Query type: $qname"  >> "$OUTPUT"
		{ time "$1" "$2" "$3" "$4" "-czero"; }  >> "$OUTPUT" 2>&1
		;;
esac