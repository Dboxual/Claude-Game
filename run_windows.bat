@echo off
setlocal
rem ==============================================================
rem  ZION - one-click build + run (Windows x64)
rem  Needs: git, CMake, Visual Studio 2022 (or Build Tools) with
rem  the "Desktop development with C++" workload.
rem  First run clones vcpkg to %USERPROFILE%\vcpkg and compiles
rem  raylib from source - expect a few minutes. Later runs are fast.
rem ==============================================================
cd /d "%~dp0"

where cmake >nul 2>nul
if not errorlevel 1 goto have_cmake
if exist "C:\Program Files\CMake\bin\cmake.exe" (
    set "PATH=C:\Program Files\CMake\bin;%PATH%"
    goto have_cmake
)
echo [error] cmake was not found. Install it with:  winget install Kitware.CMake
goto fail
:have_cmake

where git >nul 2>nul
if errorlevel 1 (
    echo [error] git was not found. Install it with:  winget install Git.Git
    goto fail
)

if defined VCPKG_ROOT if exist "%VCPKG_ROOT%\vcpkg.exe" goto have_vcpkg

if exist "%USERPROFILE%\vcpkg\vcpkg.exe" (
    set "VCPKG_ROOT=%USERPROFILE%\vcpkg"
    goto have_vcpkg
)

echo [setup] Installing vcpkg into "%USERPROFILE%\vcpkg" ...
git clone https://github.com/microsoft/vcpkg "%USERPROFILE%\vcpkg"
if errorlevel 1 goto fail
call "%USERPROFILE%\vcpkg\bootstrap-vcpkg.bat" -disableMetrics
if errorlevel 1 goto fail
set "VCPKG_ROOT=%USERPROFILE%\vcpkg"

:have_vcpkg
echo [setup] Using vcpkg at "%VCPKG_ROOT%"

echo [build] Configuring (first run also builds raylib - takes a few minutes) ...
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
if errorlevel 1 goto fail

echo [build] Compiling ...
cmake --build build --config Release --parallel
if errorlevel 1 goto fail

set "GAME=build\Release\zion.exe"
if not exist "%GAME%" set "GAME=build\zion.exe"
echo [run] %GAME% %*
"%GAME%" %*
exit /b %errorlevel%

:fail
echo.
echo Build failed - see the messages above.
if not defined CI pause
exit /b 1
