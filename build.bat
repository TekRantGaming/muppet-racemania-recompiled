@echo off
rem Build the recompiled Muppet RaceMania for one region with Visual Studio 2022
rem Build Tools + clang-cl (plain MSVC cl.exe fails on the runtime's C11 atomics).
rem   build.bat [USA|EUR]     (default USA)  ->  build-usa\ or build-eur\
rem The game code comes from generated\ (psxrecomp_cli.py generate, per region).
setlocal
set "REGION=%~1"
if "%REGION%"=="" set "REGION=USA"
if /i "%REGION%"=="USA" (set "OUT=build-usa") else if /i "%REGION%"=="EUR" (set "OUT=build-eur") else (
  echo Usage: build.bat [USA^|EUR]
  exit /b 1
)
set "PATH=C:\Program Files (x86)\Microsoft Visual Studio\Installer;%PATH%"
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
cd /d "%~dp0"
cmake -S . -B %OUT% -G Ninja -DCMAKE_BUILD_TYPE=Release -DMRM_REGION=%REGION% -DPSX_RUNTIME_IPO=ON -DCMAKE_LINKER=lld-link -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl -DCMAKE_DISABLE_FIND_PACKAGE_SDL3=ON || exit /b 1
cmake --build %OUT% --target psx-runtime || exit /b 1
echo.
echo Built %OUT% (%REGION%)
