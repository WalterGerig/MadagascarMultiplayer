@echo off
setlocal enabledelayedexpansion

echo =======================================================
echo   Madagascar Multiplayer Mod - Automated Build Script
echo =======================================================
echo.

where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo [*] CMake nicht direkt im PATH gefunden. Suche nach installierten Versionen...
    
    :: 1. Versuche CMake aus Python-Paket (requirements.txt) zu laden
    for /f "delims=" %%I in ('python -c "import cmake, os; print(os.path.normpath(cmake.CMAKE_BIN_DIR))" 2^>nul') do (
        if exist "%%I\cmake.exe" (
            set "PATH=%%I;!PATH!"
            echo  [+] Python-CMake gefunden und aktiviert: %%I
        )
    )
    
    :: 2. Versuche CMake aus Visual Studio Installationen zu laden
    where cmake >nul 2>&1
    if !errorlevel! neq 0 (
        for %%P in (
            "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"
            "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"
            "%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"
            "%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"
            "%ProgramFiles%\CMake\bin"
            "%ProgramFiles(x86)%\CMake\bin"
        ) do (
            if exist "%%~P\cmake.exe" (
                set "PATH=%%~P;!PATH!"
                echo  [+] Visual Studio / System CMake gefunden und aktiviert: %%~P
            )
        )
    )
)

where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo.
    echo =======================================================
    echo  [!] CMake 3.20+ wurde nicht gefunden!
    echo =======================================================
    echo.
    set /p AUTO_INSTALL="Moechtest du alle Anforderungen automatisch ueber install_requirements.bat installieren? (J/N): "
    if /i "!AUTO_INSTALL!"=="J" (
        call "%~dp0install_requirements.bat"
        where cmake >nul 2>&1
        if !errorlevel! neq 0 (
            echo [ERROR] CMake konnte nicht automatisch bereitgestellt werden.
            pause
            exit /b 1
        )
    ) else (
        echo Bitte installiere CMake 3.20+ oder fuehre 'install_requirements.bat' aus.
        pause
        exit /b 1
    )
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
