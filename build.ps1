#!/usr/bin/env powershell

#Remove-Item build_win -Force -Recurse -ErrorAction SilentlyContinue
New-Item -Path .\build_win -ItemType directory -Force
Set-Location build_win

cmake -G "Visual Studio 15 2017" -T "host=x64" -A "x64" "-DCMAKE_TOOLCHAIN_FILE=$env:WORKSPACE\vcpkg\scripts\buildsystems\vcpkg.cmake" "-DENABLE_CUDA:BOOL=TRUE" "-DVCPKG_TARGET_TRIPLET=x64-windows" ..
cmake --build . --target install --config Release

Set-Location ..
