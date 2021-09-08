#!/bin/bash
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
clang-format -i $(find $SCRIPT_DIR/src $SCRIPT_DIR/include -type f -name '*.cpp' -o -name '*.h' -o -name '*.hpp' -o -name '*.c')
