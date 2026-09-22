@echo off
setlocal enabledelayedexpansion

echo =======================================================
echo   Madagascar Multiplayer Mod - Automated Build Script
echo =======================================================
echo.

where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] CMake was not found in PATH!
    echo Please install CMake 3.20+ and make sure it is added to your system PATH.
    pause
    exit /b 1
)

echo [1/3] Configuring CMake project for 32-bit x86 (Win32)...
cmake -B build -A Win32
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] CMake configuration failed!
    pause
    exit /b 1
)

echo.
echo [2/3] Building Release binaries...
cmake --build build --config Release
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Compilation failed!
    pause
    exit /b 1
)

echo.
echo [3/3] Build finished successfully!
echo.
echo Built Artifacts:
if exist "build\client\Release\MadMultiplayer.dll" (
    echo  [OK] Client Mod: build\client\Release\MadMultiplayer.dll
) else (
    echo  [!] Client Mod DLL not found in expected folder.
)

if exist "build\loader\Release\d3d8.dll" (
    echo  [OK] Proxy Loader: build\loader\Release\d3d8.dll
) else (
    echo  [!] Proxy Loader d3d8.dll not found in expected folder.
)

echo.
echo Use deploy.bat to copy the files to your Madagascar game directory.
echo =======================================================
pause
