#! /bin/bash

export LD_LIBRARY_PATH=/shared/software/compilers/gcc-7.3.0_sl7/lib64:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/usr/local/cuda-10.0/lib64:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/home/HPC/afabbri/Installed/cuda-10.0/lib64:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/home/HPC/afabbri/Installed/opencv-3.4/lib64:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/home/HPC/afabbri/Installed/ffmpeg/lib:$LD_LIBRARY_PATH

cd $WORKSPACE/peoplebox
./demo $1 

