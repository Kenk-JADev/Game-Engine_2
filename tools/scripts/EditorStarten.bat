@echo off
REM ============================================================================
REM AetherRPG – Editor starten (Doppelklick)
REM Oeffnet AetherEditor mit GUI + Demo-Projekt.
REM ============================================================================
cd /d "%~dp0"

if exist "AetherEditor.exe" (
    start "" "AetherEditor.exe" --gui --project "samples\demo_project"
) else (
    echo AetherEditor.exe fehlt neben dieser Batch-Datei.
    pause
)
