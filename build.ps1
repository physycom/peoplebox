#!/usr/bin/env pwsh

$number_of_build_workers=8

function getProgramFiles32bit() {
  $out = ${env:PROGRAMFILES(X86)}
  if ($null -eq $out) {
    $out = ${env:PROGRAMFILES}
  }
  if ($null -eq $out) {
    throw "Could not find [Program Files 32-bit]"
  }
  return $out
}

function getLatestVisualStudioWithDesktopWorkloadPath() {
  $programFiles = getProgramFiles32bit
  $vswhereExe = "$programFiles\Microsoft Visual Studio\Installer\vswhere.exe"
  if (Test-Path $vswhereExe) {
    $output = & $vswhereExe -products * -latest -requires Microsoft.VisualStudio.Workload.NativeDesktop -format xml
    [xml]$asXml = $output
    foreach ($instance in $asXml.instances.instance) {
      $installationPath = $instance.InstallationPath -replace "\\$" # Remove potential trailing backslash
    }
    if (!$installationPath) {
      Write-Host "Warning: no full Visual Studio setup has been found, extending search to include also partial installations" -ForegroundColor Yellow
      $output = & $vswhereExe -products * -latest -format xml
      [xml]$asXml = $output
      foreach ($instance in $asXml.instances.instance) {
        $installationPath = $instance.InstallationPath -replace "\\$" # Remove potential trailing backslash
      }
    }
    if (!$installationPath) {
      Throw "Could not locate any installation of Visual Studio"
    }
  }
  else {
    Throw "Could not locate vswhere at $vswhereExe"
  }
  return $installationPath
}


function getLatestVisualStudioWithDesktopWorkloadVersion() {
  $programFiles = getProgramFiles32bit
  $vswhereExe = "$programFiles\Microsoft Visual Studio\Installer\vswhere.exe"
  if (Test-Path $vswhereExe) {
    $output = & $vswhereExe -products * -latest -requires Microsoft.VisualStudio.Workload.NativeDesktop -format xml
    [xml]$asXml = $output
    foreach ($instance in $asXml.instances.instance) {
      $installationVersion = $instance.InstallationVersion
    }
    if (!$installationVersion) {
      Write-Host "Warning: no full Visual Studio setup has been found, extending search to include also partial installations" -ForegroundColor Yellow
      $output = & $vswhereExe -products * -latest -format xml
      [xml]$asXml = $output
      foreach ($instance in $asXml.instances.instance) {
        $installationVersion = $instance.installationVersion
      }
    }
    if (!$installationVersion) {
      Throw "Could not locate any installation of Visual Studio"
    }
  }
  else {
    Throw "Could not locate vswhere at $vswhereExe"
  }
  return $installationVersion
}


if ((Test-Path env:VCPKG_ROOT)) {
  $vcpkg_path = "$env:VCPKG_ROOT"
  Write-Host "Found vcpkg in VCPKG_ROOT: $vcpkg_path"
}
elseif ((Test-Path "${env:WORKSPACE}\vcpkg")) {
  $vcpkg_path = "${env:WORKSPACE}\vcpkg"
  Write-Host "Found vcpkg in WORKSPACE\vcpkg: $vcpkg_path"
}
else {
  Throw "peoplebox is unsupported without vcpkg!"
}

if ($null -eq $env:VCPKG_DEFAULT_TRIPLET) {
  Write-Host "No default triplet has been set-up for vcpkg. Defaulting to x64-windows" -ForegroundColor Yellow
  $vcpkg_triplet = "x64-windows"
}
else {
  $vcpkg_triplet = $env:VCPKG_DEFAULT_TRIPLET
}

if ($vcpkg_triplet -Match "x86") {
  Throw "peoplebox is supported only in x64 builds!"
}

if ($null -eq (Get-Command "cl.exe" -ErrorAction SilentlyContinue)) {
  $vsfound = getLatestVisualStudioWithDesktopWorkloadPath
  Write-Host "Found VS in ${vsfound}"
  Push-Location "${vsfound}\Common7\Tools"
  cmd.exe /c "VsDevCmd.bat -arch=x64 & set" |
  ForEach-Object {
    if ($_ -match "=") {
      $v = $_.split("="); Set-Item -force -path "ENV:\$($v[0])"  -value "$($v[1])"
    }
  }
  Pop-Location
  Write-Host "Visual Studio Command Prompt variables set" -ForegroundColor Yellow
}

$tokens = getLatestVisualStudioWithDesktopWorkloadVersion
$tokens = $tokens.split('.')
if ($tokens[0] -eq "14") {
  $generator = "Visual Studio 14 2015"
}
elseif ($tokens[0] -eq "15") {
  $generator = "Visual Studio 15 2017"
}
elseif ($tokens[0] -eq "16") {
  $generator = "Visual Studio 16 2019"
}
else {
  throw "Unknown Visual Studio version, unsupported configuration"
}
Write-Host "Setting up environment to use CMake generator: $generator" -ForegroundColor Yellow

if ($null -eq (Get-Command "nvcc.exe" -ErrorAction SilentlyContinue)) {
  if (Test-Path env:CUDA_PATH) {
    $env:PATH += ";${env:CUDA_PATH}\bin"
    Write-Host "Found cuda in ${env:CUDA_PATH}" -ForegroundColor Yellow
  }
  else {
    Write-Host "Unable to find CUDA, if necessary please install it or define a CUDA_PATH env variable pointing to the install folder" -ForegroundColor Yellow
  }
}

if (Test-Path env:CUDA_PATH) {
  if (-Not(Test-Path env:CUDA_TOOLKIT_ROOT_DIR)) {
    $env:CUDA_TOOLKIT_ROOT_DIR = "${env:CUDA_PATH}"
    Write-Host "Added missing env variable CUDA_TOOLKIT_ROOT_DIR" -ForegroundColor Yellow
  }
  if ($my_cuda_compute_model) {
    $additional_build_setup = "-DCUDA_COMPUTE_MODEL=${my_cuda_compute_model}"
    Write-Host "Using compute capability ${my_cuda_compute_model}" -ForegroundColor Yellow
  }
  else {
    Write-Host "Using default compute capability" -ForegroundColor Yellow
  }
}


## DEBUG
#New-Item -Path .\build_win_debug -ItemType directory -Force
#Set-Location build_win_debug
#cmake -G "$generator" -T "host=x64" -A "x64" "-DCMAKE_TOOLCHAIN_FILE=$vcpkg_path\scripts\buildsystems\vcpkg.cmake" "-DVCPKG_TARGET_TRIPLET=$vcpkg_triplet" #"-DCMAKE_BUILD_TYPE=Debug" $additional_build_setup ..
#cmake --build . --config Debug --parallel ${number_of_build_workers} --target install  #valid only for CMake 3.12+
#Set-Location ..

# RELEASE
New-Item -Path .\build_win_release -ItemType directory -Force
Set-Location build_win_release
cmake -G "$generator" -T "host=x64" -A "x64" "-DCMAKE_TOOLCHAIN_FILE=$vcpkg_path\scripts\buildsystems\vcpkg.cmake" "-DVCPKG_TARGET_TRIPLET=$vcpkg_triplet" "-DCMAKE_BUILD_TYPE=Release" $additional_build_setup ..
cmake --build . --config Release --parallel ${number_of_build_workers} --target install  #valid only for CMake 3.12+
Set-Location ..
