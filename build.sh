#!/bin/bash
number_of_build_workers=8

if [[ "$OSTYPE" == "darwin"* ]]; then
  build_dir=build_macos
else
  build_dir=build_lin
fi

if [[ $(hostname) == *"hpc"* ]]; then
  module purge
  module load compilers/gcc-7.3.0_sl7
  module load compilers/cuda-10.0
  export CUDNN=/home/HPC/afabbri/Installed/cuda-10.0
  export CC=$(which gcc)
  export CXX=$(which g++)
  export CUDA_TOOLKIT_ROOT_DIR=/usr/local/cuda-10.0/
  export CUDA_PATH=/usr/local/cuda-10.0/
  export OpenCV_DIR=/home/HPC/afabbri/Installed/opencv-3.4
  additional_defines="${additional_defines} -DOVERRIDE_GCC_OPTIM=-ffp-contract=fast,-march=x86-64"
  export LD_LIBRARY_PATH=/home/HPC/afabbri/Installed/ffmpeg/lib:$LD_LIBRARY_PATH
fi

if [[ "$OSTYPE" == "darwin"* && "$1" == "gcc" ]]; then
  export CC="/usr/local/bin/gcc-8"
  export CXX="/usr/local/bin/g++-8"
fi

if [[ $(hostname) == *"jetson"* ]]; then
  additional_defines="${additional_defines} -DOVERRIDE_GCC_OPTIM=-ffp-contract=fast"
fi

mkdir -p $build_dir
cd $build_dir
cmake .. -DCMAKE_BUILD_TYPE=Release ${additional_defines}
cmake --build . --target install --parallel ${number_of_build_workers}
cd ..
