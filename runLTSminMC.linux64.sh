#!/bin/bash
gcc -g -O3 -c -I lib/LTSmin/include/ -I autogenerated -std=c99 -fPIC autogenerated/dlopen-impl.c -o autogenerated/dlopen-impl.o
gcc -g -c -I lib/LTSmin/include -I autogenerated -std=c99 -fPIC autogenerated/autogeneratedfile.c -o autogenerated/autogeneratedfile.o

gcc -g -shared -o autogenerated/sharedfile.so autogenerated/dlopen-impl.o autogenerated/autogeneratedfile.o

lib/LTSmin/bin/pins2lts-seq --state=table --invariant="! goal" --when autogenerated/sharedfile.so
#lib/LTSmin/bin/pins2lts-seq --state=table --invariant="! goal" --trace=solution.gcf --when autogenerated/sharedfile.so 

#lib/LTSmin/bin/pins2lts-mc --procs=2 --state=table --invariant="! goal" --trace=solution.gcf --when autogenerated/sharedfile.so 

#Setting for query: --invariant="! goal" 
#Setting for tracelog: --trace=solution.gcf
#Setting for deadlock: --deadlock
#Setting for benchmark-test: --when
#Setting for use of table instead of tree: --state=table

#Command for LTSmin multi-core: pins2lts-mc
#Command for LTSmin singlecore-core: pins2lts-seq

#Pretty Print tracelog: ltsmin-printtrace solution.gcf