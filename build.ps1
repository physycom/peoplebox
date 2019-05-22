#!/usr/bin/env powershell

$number_of_build_workers = 12
New-Item -Path .\build_win -ItemType directory -Force
Set-Location build_win

$my_cuda_compute_model = 75    #Compute capability for Tesla T4, RTX 2080
$additional_build_setup = "-DCUDA_COMPUTE_MODEL=${my_cuda_compute_model}"

cmake -G "Visual Studio 15 2017" -T "host=x64" -A "x64" "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" "-DVCPKG_TARGET_TRIPLET=x64-windows" "-DCMAKE_BUILD_TYPE=Release" $additional_build_setup ..
cmake --build . --config Release --parallel ${number_of_build_workers} --target install
#Copy-Item *.dll ..
Set-Location ..
