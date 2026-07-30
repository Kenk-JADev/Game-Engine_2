# AetherRPG Maker

Eigenständige **3D-RPG-Maker-Software** mit eigener Engine, eigenem Editor und schlanker Runtime (`Game` / `Game.exe`).

> Keine Abhängigkeit von Unity, Unreal, Godot oder proprietärem RGSS-Quellcode.

## Komponenten

| Target | Beschreibung |
|--------|--------------|
| **aether_engine** | C++20-Engine (Core, Window, Render, Input, Audio, Resources, Ruby, Events, DB, Plugins) |
| **AetherEditor** | Editor-Konzept: Projekt · Karte · Datenbank · Events · Skripte · Testspiel · Export |
| **Game** | Runtime – lädt Projekt und startet das Spiel |

## Designziele

- Anfänger erstellen RPGs **ohne Programmierung** (Events, Datenbank, Drag & Drop).
- Fortgeschrittene nutzen **Ruby** für Spiellogik und Plugins.
- Zielperformance: **60 FPS** auf älteren PCs (i3-4xxx / HD 4600, 8 GB RAM).
- Stilisierte 3D-RPGs – Forward-Rendering, Frustum-Culling, LOD, kein Raytracing.
- **Keine** Unity-Style Component-/Collider-UI im Editor.

## Build

Voraussetzungen: CMake ≥ 3.21, C++20-Compiler, Git.  
Optional für echtes Fenster: `libx11-dev libgl1-mesa-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev`.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Binaries: `build/bin/AetherEditor`, `build/bin/Game`

### Runtime

```bash
./build/bin/Game --project samples/demo_project --headless --max-frames 5
./build/bin/Game --project /path/MyGame
```

### Editor (CLI-Phase)

```bash
./build/bin/AetherEditor --new /tmp/MyGame
./build/bin/AetherEditor --project /tmp/MyGame --testplay
```

## Modulstatus

| # | Modul | Status |
|---|-------|--------|
| 1–3 | Architektur, Ordner, Build | ✅ |
| 4 | Core (Logger, Time, Config, ThreadPool, EventBus, Context) | ✅ |
| 5 | Fenster (Null + optional GLFW) | ✅ |
| 6 | Renderer (Culling, LOD, Shader-Quellen; Null-Backend) | ✅ |
| 7 | Input (Action-Mapping RPG-Stil) | ✅ |
| 8 | Audio (BGM/BGS/ME/SE) | ✅ |
| 9 | Ressourcen (VFS, Cache, JSON) | ✅ |
| 10 | Ruby (Stub-VM + Engine-API-Module) | ✅ |
| 11 | Editor CLI | ✅ |
| 12 | Runtime Game | ✅ |
| 13 | Eventsystem (Interpreter + JSON) | ✅ |
| 14 | Datenbank (actors/enemies/items/skills) | ✅ |
| 15 | Export-Paket | ✅ |
| 16 | Plugin-Loader | ✅ |
| 17 | Tests (11 automatisiert) | ✅ |

## Nächste Ausbaustufen

- OpenGL-3.3-Renderer-Pfad (GLAD) bei vorhandenem Display
- mruby statt Stub-VM
- miniaudio / stb_image / assimp
- Editor-GUI (Karten-View, DB-Tabs, visueller Event-Editor, Script-IDE)
- Physik-Kollision & Navigation automatisch beim Objekt-Drop

## Dokumentation

Siehe [`docs/architecture/`](docs/architecture/) – ein Dokument pro Modul.

## Lizenz

MIT – siehe [LICENSE](LICENSE).
