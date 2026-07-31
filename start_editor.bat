@echo off
setlocal

REM AetherRPG Maker - Windows Quick Launcher
REM This script tries to find and start the Editor.
REM For the easiest experience, download the ready package from:
REM https://github.com/Kenk-JADev/Game-Engine_2/releases

set "SCRIPT_DIR=%~dp0"

REM Try common build locations
if exist "%SCRIPT_DIR%build-release\bin\AetherEditor.exe" (
    cd /d "%SCRIPT_DIR%build-release\bin"
    start "" "AetherEditor.exe" --gui %*
    goto :eof
)

if exist "%SCRIPT_DIR%build\bin\AetherEditor.exe" (
    cd /d "%SCRIPT_DIR%build\bin"
    start "" "AetherEditor.exe" --gui %*
    goto :eof
)

if exist "%SCRIPT_DIR%bin\AetherEditor.exe" (
    cd /d "%SCRIPT_DIR%bin"
    start "" "AetherEditor.exe" --gui %*
    goto :eof
)

echo.
echo AetherRPG Maker Editor not found in common locations.
echo.
echo Recommended way (no build needed):
echo   1. Go to https://github.com/Kenk-JADev/Game-Engine_2/releases
echo   2. Download the latest "AetherRPG-Maker-Windows-x64.zip"
echo   3. Extract and double-click "start_editor.bat" inside the extracted folder.
echo.
pause
endlocal
