call "%PROGRAMFILES(x86)%\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

rmdir /S /Q build
mkdir build
cd build
REM cmake -G "Ninja" "-DCMAKE_TOOLCHAIN_FILE=%WORKSPACE%\vcpkg_opencv2\scripts\buildsystems\vcpkg.cmake" "-DVCPKG_TARGET_TRIPLET=%VCPKG_DEFAULT_TRIPLET%" "-DCMAKE_BUILD_TYPE=Release" ..
cmake -G "Ninja" "-DCMAKE_TOOLCHAIN_FILE=%WORKSPACE%\vcpkg_opencv2\scripts\buildsystems\vcpkg.cmake" "-DVCPKG_TARGET_TRIPLET=%VCPKG_DEFAULT_TRIPLET%" "-DCMAKE_BUILD_TYPE=Debug" ..
cmake --build . --config Release
cd ..
