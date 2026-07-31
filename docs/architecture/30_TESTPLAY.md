# Modul 30 – Echtes Testspiel (Runtime-Subprozess)

**Status:** implementiert

---

## 1. Ziel

Der „Testspiel“-Tab führte bisher nur einen Headless-Smoke aus
(`run_testplay_smoke`: Skript laden, 5 Frames simulieren, Nav-Grid prüfen).
Das entspricht nicht dem RPG-Maker-Workflow: **Testspiel öffnet das Projekt
in der echten Runtime** mit eigenem Fenster, echter Eingabe, echtem Audio.

## 2. Architektur

```
Editor (Testspiel-Tab / F5)
   │  1. save_project()  – aktuelle Karte, Skripte, Datenbank sichern
   │  2. find_game_binary() – Game/Game.exe neben der Editor-Executable
   │  3. start_testplay(project_dir)
   ▼
std::system(build_testplay_command(...))
   Windows:  start "AetherRPG Testspiel" "Game.exe" --project "dir"
   POSIX:    "Game" --project "dir" >/dev/null 2>&1 &   (detached)
   ▼
Game-Runtime (eigener Prozess, eigenes Fenster)
```

## 3. Dateien

| Datei | Inhalt |
|-------|--------|
| `editor/src/testplay/testplay_runner.hpp/.cpp` | `shell_quote`, `executable_dir`, `find_game_binary`, `build_testplay_command`, `start_testplay` |
| `editor_app.cpp` | Testspiel-Tab: **„Testspiel starten“** (echt) + „Headless-Smoke“; Menü F5 startet ebenfalls die echte Runtime (im Headless-CLI bleibt der Smoke) |

## 4. Verhalten

- Vor dem Start wird `save_project()` aufgerufen → das Testspiel sieht den
  aktuellen Stand (Karte, Skripte, Datenbank).
- Das Testspiel läuft **unabhängig vom Editor**; zum Beenden reicht das
  Schließen des Spiel-Fensters.
- Fehlt das Game-Binary, erscheint eine klare Meldung mit Build-Hinweis.

## 5. Tests

`tests/unit/testplay_test.cpp` (Test #26 `aether_testplay_test`):

- `shell_quote` (Leerzeichen, Anführungszeichen)
- `build_testplay_command` (enthält `--project` + quotierte Pfade; Windows-`start` / POSIX-Detach)
- `executable_dir`/`find_game_binary`: findet die Game-Runtime im Build-Bin-Verzeichnis
  (der Test läuft neben `Game`), sonst leerer Pfad ohne Fehler

Der echte Prozessstart wird im Test bewusst nicht ausgelöst (würde ein
Spiel-Fenster öffnen); er nutzt nur plattformstandard `std::system`.

---

*Nächstes Modul: Script-Editor als echte IDE*
