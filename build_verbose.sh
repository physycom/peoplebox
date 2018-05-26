#!/bin/bash

if [[ "$OSTYPE" == "darwin"* && "$1" == "gcc" ]]; then
  export CC="/usr/local/bin/gcc-7"
  export CXX="/usr/local/bin/g++-7"
  remote="$2"
else
  remote="$1"
fi

./clean.sh
mkdir -p build
cd build
cmake .. -DVERBOSE:BOOL=TRUE
cmake --build . --target install
cd ..
