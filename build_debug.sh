#!/bin/bash

if [[ "$OSTYPE" == "darwin"* && "$1" == "gcc" ]]; then
 export CC="/usr/local/bin/gcc-8"
 export CXX="/usr/local/bin/g++-8"
fi

./clean.sh
#rm -rf build_debug
mkdir -p build_debug
cd build_debug
cmake .. -DCMAKE_BUILD_TYPE=Debug -DVERBOSE:BOOL=TRUE
cmake --build .
cd ..
