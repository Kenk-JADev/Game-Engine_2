# Editor mit GUI bauen und starten (Fenster + ImGui)

**Ziel:** Der Editor läuft im Vollmodus mit Fenster, ImGui-Tabs
(Projekt | Karte | Datenbank | Events | Skripte | Testspiel | Export)
und 3D-Viewport.

## Woran erkenne ich, ob der Build die GUI hat?

Beim Start im GUI-Modus MÜSSEN diese Log-Zeilen erscheinen:

```
[INFO] [Window] GLFW initialized
[INFO] [Window] GLFW window created 1280x720 (fb 1280x720)
[INFO] [Renderer] GlRenderer ready (OpenGL 3.3)
[INFO] [Editor] ImGui initialized
```

**Falsch** (dann ist der Build headless gebaut oder headless gestartet):

```
[INFO] [Window] Null window system initialized
[INFO] [Renderer] NullRenderer created (headless)
[INFO] [Editor] ImGui not linked – headless/CLI UI mode
```

Seit Commit `6e1abe8` bricht der Editor in diesem Fall mit einer klaren
Fehlermeldung ab (statt still weiterzulaufen), wenn die GUI erwartet wird.

---

## Weg 1 – Fertige Binaries von GitHub Actions (schnellster Weg)

Die CI baut bei jedem Push **Windows- und Linux-Binaries mit GUI** und lädt
sie als Artifakte hoch:

1. **Actions** öffnen: `https://github.com/Kenk-JADev/Game-Engine_2/actions`
2. Den neuesten **grünen** Lauf anklicken (Name = letzter Commit)
3. Ganz unten unter **Artifacts**:
   - **Windows:** `aether-windows-x64` herunterladen
   - **Linux:** `aether-linux-x64` herunterladen
4. ZIP entpacken → enthält `AetherEditor.exe` / `AetherEditor`,
   `Game.exe` / `Game`, `assets/`, `templates/`, `samples/demo_project/`
5. Starten:

```bash
# Windows (Ordner der entpackten ZIP):
AetherEditor.exe --gui --project samples\demo_project

# Linux:
./AetherEditor --gui --project samples/demo_project
```

> Die ZIP enthält alle Engine-Assets (Shader, Texturen) und das
> Demo-Projekt – nichts muss extra installiert werden.

---

## Weg 2 – Lokal bauen (mit GUI-Flags)

### Windows (Visual Studio 2022, x64)

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 ^
  -DAETHER_WITH_GLFW=ON -DAETHER_WITH_OPENGL=ON ^
  -DAETHER_WITH_MINIAUDIO=ON -DAETHER_WITH_STB=ON
cmake --build build --config Release
build\bin\Release\AetherEditor.exe --gui --project samples\demo_project
```

(Keine Zusatz-Installation nötig: GLFW kommt via FetchContent, OpenGL ist
System-Bestandteil. Erforderlich: Git + CMake + VS-Desktop-C++-Workload.)

### Linux (Ubuntu/Debian)

```bash
sudo apt install cmake ninja-build git build-essential \
  libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
  libgl1-mesa-dev libglu1-mesa-dev mesa-common-dev \
  libasound2-dev libpulse-dev

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DAETHER_WITH_GLFW=ON -DAETHER_WITH_OPENGL=ON \
  -DAETHER_WITH_MINIAUDIO=ON -DAETHER_WITH_STB=ON
cmake --build build
./build/bin/AetherEditor --gui --project samples/demo_project
```

### Wichtig: CMake-Cache

Wenn du vorher einmal headless konfiguriert hast (`-DAETHER_WITH_GLFW=OFF`
oder fehlende X11-Pakete), bleibt das im Cache. **Build-Ordner löschen**
oder neu konfigurieren:

```bash
rm -rf build
cmake -S . -B build ...
```

Kontrolle nach dem Configure: Es MUSS stehen

```
-- Aether Window backend: GLFW
-- Aether Renderer: OpenGL 3.3 + Null
```

Steht dort `Null (headless)`, fehlen die Abhängigkeiten (bei Linux: X11-Dev-
Pakete installieren und neu konfigurieren).

---

## Weg 3 – Release-Workflow aktivieren (dauerhafte Downloads)

Die Datei `docs/dev/github-workflows/release.yml` erzeugt bei einem Git-Tag
`v*` Release-Archive (Windows + Linux) inkl. Editor und Runtime. Sie ist
noch nicht aktiv, weil `.github/workflows/` eine `workflows`-Permission
braucht. Aktiviere sie einmalig mit deinem eigenen Token:

```bash
cp docs/dev/github-workflows/release.yml .github/workflows/release.yml
git add .github/workflows/release.yml
git commit -m "ci: enable release packaging"
git push
```

Danach ein Tag setzen, um die Release-Archive zu bauen:

```bash
git tag v0.1.0
git push origin v0.1.0
```

GitHub erstellt dann unter **Releases** ein `v0.1.0` mit `aether-windows-x64`
und `aether-linux-x64` als Download-Anhänge.

---

## Troubleshooting

| Symptom | Ursache / Lösung |
|---------|------------------|
| `NullWindow created (headless)` beim GUI-Start | Build ohne GLFW/OpenGL → Weg 2 (Flags, Cache löschen) oder Weg 1 (fertige Binaries) |
| `ImGui not linked` | gleiche Ursache – ImGui braucht GLFW+OpenGL |
| `AetherEditor.exe` startet und schließt sofort | Im GUI-Modus ohne Display/Desktop (z. B. RDP ohne GPU) → dann `--headless` nutzen oder Xvfb |
| GLFW-Configure-Fehler `Failed to find wayland-scanner` | bekanntes GLFW-3.4-Problem; der Workflow setzt `GLFW_BUILD_WAYLAND=OFF` bereits |
| „GUI requested, but this build has no GLFW/OpenGL“ | exakt dieser Fall – siehe Weg 1/2 |


---

## Game.exe starten (Runtime)

Die Runtime braucht ein Projekt (project.json). Zwei Wege:

1. **Doppelklick auf Game.exe** – sie sucht `project.json` im selben Ordner
   (Fallback seit Commit `3b44285`). Ein exportiertes Spiel-Paket enthält
   `Game.exe` + `project.json` + `data/` + `maps/` + … im selben Ordner –
   Doppelklick startet das Spiel direkt.

2. **Mit Pfad**:
   ```
   Game.exe --project C:\MeinSpiel
   ```

**Fehlt project.json**, erscheint jetzt eine klare Fehlermeldung (Windows:
Dialogbox) statt dass das Fenster stumm verschwindet.
