#!/bin/bash

if [[ "$OSTYPE" == "darwin"* && "$1" == "gcc" ]]; then
  export CC="/usr/local/bin/gcc-7"
  export CXX="/usr/local/bin/g++-7"
  remote="$2"
else
  remote="$1"
fi

mkdir -p build
cd build
cmake ..
cmake --build . --target install
cd ..

if [[ $remote != "" ]]; then
  cd build
  cmake -D remote=$remote ..
  cmake --build . --target deploy
  cd ..
fi
