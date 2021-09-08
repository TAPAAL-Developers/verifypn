#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
F=$(find $SCRIPT_DIR/src $SCRIPT_DIR/include -type f -name '*.cpp' -o -name '*.h' -o -name '*.hpp' -o -name '*.c' | grep -v ".lexer." | grep -v ".parser.")
clang-tidy $F -fix -fix-errors -extra-arg-before=-xc++-header -extra-arg=-std=c++17 -extra-arg=-I./$SCRIPT_DIR/include -extra-arg=-I$SCRIPT_DIR/build/external/include -p $SCRIPT_DIR/build/compile_commands
