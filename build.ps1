#!/usr/bin/env powershell

#Remove-Item build -Force -Recurse -ErrorAction SilentlyContinue
New-Item -Path .\build_win -ItemType directory -Force
Set-Location build_win

cmake -G "Visual Studio 15 2017" -T "host=x64" -A "x64" "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" "-DVCPKG_TARGET_TRIPLET=x64-windows" "-DCMAKE_BUILD_TYPE=Release" ..
cmake --build . --config Release --target install
Set-Location ..
