@echo off
rem Build the launcher (build-launcher\MuppetRaceMania.exe): TRG Launcher 2.0 from third_party\trg-launcher,
rem or from the folder in TRG_LAUNCHER_DIR. Visual Studio 2022 Build Tools.
setlocal
set "PATH=C:\Program Files (x86)\Microsoft Visual Studio\Installer;%PATH%"
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
cd /d "%~dp0"
set "TRG_ARG="
if defined TRG_LAUNCHER_DIR set TRG_ARG="-DTRG_LAUNCHER_DIR=%TRG_LAUNCHER_DIR%"
cmake -S launcher -B build-launcher -G Ninja -DCMAKE_BUILD_TYPE=Release %TRG_ARG% || exit /b 1
cmake --build build-launcher --target MuppetRaceMania || exit /b 1
echo Built build-launcher\MuppetRaceMania.exe
