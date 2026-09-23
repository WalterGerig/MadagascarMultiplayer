@echo off
setlocal enabledelayedexpansion

title Madagascar Multiplayer - Requirements Installer
echo =======================================================
echo   Madagascar Multiplayer Mod - Requirements Installer
echo =======================================================
echo.

:: 1. Pruefe Python Installation
echo [1/4] Ueberpruefe Python...
where python >nul 2>&1
if %errorlevel% neq 0 (
    echo [*] Python wurde nicht im PATH gefunden!
    where winget >nul 2>&1
    if %errorlevel% equ 0 (
        echo Winget wurde gefunden. Installiere Python 3.11 automatisch...
        winget install -e --id Python.Python.3.11 --accept-source-agreements --accept-package-agreements
        echo Bitte starte dieses Skript nach der Python-Installation erneut.
        pause
        exit /b 1
    ) else (
        echo [FEHLER] Bitte installiere Python 3.10+ von https://www.python.org/
        echo und aktiviere bei der Installation die Option "Add python.exe to PATH".
        pause
        exit /b 1
    )
)
python --version

:: 2. Pip Pakete und CMake via requirements.txt installieren
echo.
echo [2/4] Installiere Abhaengigkeiten aus requirements.txt...
python -m pip install -r "%~dp0requirements.txt"
if %errorlevel% neq 0 (
    echo.
    echo [WARNUNG] Ein Fehler ist bei 'pip install' aufgetreten.
)

:: 3. CMake Verzeichnis automatisch in PATH einbinden
echo.
echo [3/4] Konfiguriere CMake und Benutzer-PATH...
python -c "import os, sys, winreg; from cmake import CMAKE_BIN_DIR; p = os.path.normpath(CMAKE_BIN_DIR); s = os.path.normpath(os.path.join(os.environ.get('APPDATA', ''), 'Python', f'Python{sys.version_info.major}{sys.version_info.minor}', 'Scripts')); key = winreg.OpenKey(winreg.HKEY_CURRENT_USER, r'Environment', 0, winreg.KEY_READ | winreg.KEY_WRITE); cur, _ = winreg.QueryValueEx(key, 'Path'); paths = [x for x in cur.split(';') if x]; [paths.append(d) for d in (p, s) if os.path.isdir(d) and d not in paths]; winreg.SetValueEx(key, 'Path', 0, winreg.REG_EXPAND_SZ, ';'.join(paths)); winreg.CloseKey(key)" >nul 2>&1

:: Laufenden Session-PATH aktualisieren
for /f "delims=" %%I in ('python -c "import cmake, os; print(os.path.normpath(cmake.CMAKE_BIN_DIR))" 2^>nul') do (
    set "PATH=%%I;!PATH!"
)
for /f "delims=" %%I in ('python -c "import sys, os; print(os.path.normpath(os.path.join(os.environ.get('APPDATA', ''), 'Python', f'Python{sys.version_info.major}{sys.version_info.minor}', 'Scripts')))" 2^>nul') do (
    set "PATH=%%I;!PATH!"
)

:: Teste CMake Aufruf
where cmake >nul 2>&1
if %errorlevel% equ 0 (
    echo  [OK] CMake erfolgreich bereitgestellt:
    cmake --version | findstr /I "cmake version"
) else (
    echo  [HINWEIS] Bitte oeffne nach der Installation ein neues CMD-Fenster.
)

:: 4. Visual Studio C++ Compiler pruefen
echo.
echo [4/4] Ueberpruefe Visual Studio C++ Build Tools...
set "HAS_MSVC=0"
set "VSWHERE_EXE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if exist "%VSWHERE_EXE%" (
    "%VSWHERE_EXE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath > "%TEMP%\mad_vswhere.tmp" 2>nul
    if exist "%TEMP%\mad_vswhere.tmp" (
        for /f "usebackq delims=" %%i in ("%TEMP%\mad_vswhere.tmp") do (
            if not "%%i"=="" (
                set "HAS_MSVC=1"
                echo  [OK] Visual Studio C++ Compiler gefunden in: %%i
            )
        )
        del "%TEMP%\mad_vswhere.tmp" >nul 2>&1
    )
)

if "!HAS_MSVC!"=="0" goto :no_msvc
goto :msvc_ok

:no_msvc
echo [*] Kein Visual Studio MSVC C++ Compiler gefunden!
echo Zum Kompilieren der Mod wird Visual Studio 2019/2022 mit Desktop-Entwicklung mit C++ benoetigt.
where winget >nul 2>&1
if %errorlevel% equ 0 (
    set /p INSTALL_VS="Moechtest du Visual Studio 2022 Build Tools jetzt ueber winget installieren? [J/N]: "
    if /i "!INSTALL_VS!"=="J" (
        echo Starte Installation der VS 2022 Build Tools...
        winget install -e --id Microsoft.VisualStudio.2022.BuildTools --override "--passive --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
    )
)

:msvc_ok
echo.
echo =======================================================
echo   Alle Anforderungen wurden erfolgreich eingerichtet!
echo =======================================================
echo.
set /p RUN_BUILD="Moechtest du das Projekt jetzt direkt bauen mit build.bat? [J/N]: "
if /i "!RUN_BUILD!"=="J" (
    call "%~dp0build.bat"
)

pause
