#!/usr/bin/env bash

if [[ "$OSTYPE" == "darwin"* && "$1" == "gcc" ]]; then
  export CC="/usr/local/bin/gcc-8"
  export CXX="/usr/local/bin/g++-8"
fi

./clean.sh

if [[ "$1" == "gpu" ]]; then
  mkdir -p build_gpu
  cd build_gpu
  cmake -DENABLE_CUDA:BOOL=TRUE -DFORCE_CUSTOM_OPENCV:BOOL=TRUE ..
  cmake --build . --target install
  cd ..
else
  mkdir -p build
  cd build
  cmake -DFORCE_CUSTOM_OPENCV:BOOL=TRUE ..
  cmake --build . --target install
  cd ..
fi

