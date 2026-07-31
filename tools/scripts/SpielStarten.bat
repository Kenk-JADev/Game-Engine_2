@echo off
REM ============================================================================
REM AetherRPG – Spiel starten (Doppelklick)
REM Startet Game.exe mit dem Demo-Projekt (relativ zu diesem Ordner).
REM Fuer ein eigenes Spiel:  Game.exe --project "Pfad\zu\deinem\Spiel"
REM ============================================================================
cd /d "%~dp0"

if exist "Game.exe" (
    if exist "samples\demo_project\project.json" (
        start "" "Game.exe" --project "samples\demo_project"
    ) else if exist "project.json" (
        start "" "Game.exe"
    ) else (
        echo Kein Projekt gefunden.
        echo Starte:  Game.exe --project "Pfad\zu\deinem\Spiel"
        pause
    )
) else (
    echo Game.exe fehlt neben dieser Batch-Datei.
    pause
)
