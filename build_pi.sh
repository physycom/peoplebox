#!/bin/bash

if [[ "$OSTYPE" == "darwin"* && "$1" == "gcc" ]]; then
  export CC="/usr/local/bin/gcc-8"
  export CXX="/usr/local/bin/g++-8"
fi

./clean.sh
mkdir -p build
cd build
cmake .. -DFORCE_LOCAL_OPENCV:BOOL=TRUE
cmake --build . --target install
cd ..
