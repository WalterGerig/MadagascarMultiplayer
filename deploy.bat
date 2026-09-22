@echo off
setlocal enabledelayedexpansion

echo =======================================================
echo   Madagascar Multiplayer Mod - Deployment Script
echo =======================================================
echo.

set "DLL_PATH="
if exist "build\client\Release\MadMultiplayer.dll" (
    set "DLL_PATH=build\client\Release\MadMultiplayer.dll"
) else if exist "client\build\Release\MadMultiplayer.dll" (
    set "DLL_PATH=client\build\Release\MadMultiplayer.dll"
)

set "LOADER_PATH="
if exist "build\loader\Release\d3d8.dll" (
    set "LOADER_PATH=build\loader\Release\d3d8.dll"
) else if exist "external github tools\MadagascarPatchLoader-main\MadagascarPatchLoader-main\MadLoader\build\Release\d3d8.dll" (
    set "LOADER_PATH=external github tools\MadagascarPatchLoader-main\MadagascarPatchLoader-main\MadLoader\build\Release\d3d8.dll"
)

if "%DLL_PATH%"=="" (
    echo [ERROR] MadMultiplayer.dll not found! Run build.bat first.
    pause
    exit /b 1
)

set "TARGET_PROGFILES=C:\Program Files (x86)\Activision\Madagascar\Game"
set "TARGET_VIRTUAL=%LOCALAPPDATA%\VirtualStore\Program Files (x86)\Activision\Madagascar\Game"

echo Deploying to VirtualStore directory:
echo   %TARGET_VIRTUAL%
echo.

if not exist "%TARGET_VIRTUAL%\patches" mkdir "%TARGET_VIRTUAL%\patches"
copy /Y "%DLL_PATH%" "%TARGET_VIRTUAL%\patches\MadMultiplayer.dll"
if exist "config\multiplayer_config.ini" (
    if not exist "%TARGET_VIRTUAL%\patches\multiplayer_config.ini" (
        copy /Y "config\multiplayer_config.ini" "%TARGET_VIRTUAL%\patches\multiplayer_config.ini"
    )
)
if not "%LOADER_PATH%"=="" (
    copy /Y "%LOADER_PATH%" "%TARGET_VIRTUAL%\d3d8.dll"
)

echo.
echo Deploying to Program Files directory (if accessible):
echo   %TARGET_PROGFILES%
echo.

if exist "%TARGET_PROGFILES%" (
    if not exist "%TARGET_PROGFILES%\patches" mkdir "%TARGET_PROGFILES%\patches" 2>nul
    copy /Y "%DLL_PATH%" "%TARGET_PROGFILES%\patches\MadMultiplayer.dll" 2>nul
    if not "%LOADER_PATH%"=="" (
        copy /Y "%LOADER_PATH%" "%TARGET_PROGFILES%\d3d8.dll" 2>nul
    )
    if exist "config\multiplayer_config.ini" (
        if not exist "%TARGET_PROGFILES%\patches\multiplayer_config.ini" (
            copy /Y "config\multiplayer_config.ini" "%TARGET_PROGFILES%\patches\multiplayer_config.ini" 2>nul
        )
    )
)

echo =======================================================
echo Deployment completed!
echo =======================================================
pause
