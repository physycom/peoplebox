#!/usr/bin/env powershell

#Remove-Item build -Force -Recurse -ErrorAction SilentlyContinue
New-Item -Path .\build -ItemType directory -Force
Set-Location build

cmake -G "Ninja" "-DCMAKE_TOOLCHAIN_FILE=$env:WORKSPACE\vcpkg_opencv2\scripts\buildsystems\vcpkg.cmake" "-DVCPKG_TARGET_TRIPLET=$env:VCPKG_DEFAULT_TRIPLET" "-DCMAKE_BUILD_TYPE=Release" ..
cmake --build . --config Release

Set-Location ..
