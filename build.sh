#!/bin/bash

if [[ "$OSTYPE" == "darwin"* ]]; then
  build_dir=build_macos
else
  build_dir=build_lin
fi


if [[ "$OSTYPE" == "darwin"* && "$1" == "gcc" ]]; then
  export CC="/usr/local/bin/gcc-8"
  export CXX="/usr/local/bin/g++-8"
fi

mkdir -p $build_dir
cd $build_dir
cmake ..
cmake --build . --target install
cd ..
