@echo off
rem Debug-tools build (TCP debug server, --debug-port 4370) for one region:
rem   build-dev.bat [USA|EUR]   ->  build-dev-usa\ or build-dev-eur\
setlocal
set "REGION=%~1"
if "%REGION%"=="" set "REGION=USA"
if /i "%REGION%"=="USA" (set "OUT=build-dev-usa") else if /i "%REGION%"=="EUR" (set "OUT=build-dev-eur") else (
  echo Usage: build-dev.bat [USA^|EUR]
  exit /b 1
)
set "PATH=C:\Program Files (x86)\Microsoft Visual Studio\Installer;%PATH%"
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
cd /d "%~dp0"
cmake -S . -B %OUT% -G Ninja -DCMAKE_BUILD_TYPE=Release -DMRM_REGION=%REGION% -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl -DCMAKE_DISABLE_FIND_PACKAGE_SDL3=ON -DPSX_DEBUG_TOOLS=ON || exit /b 1
cmake --build %OUT% --target psx-runtime || exit /b 1
