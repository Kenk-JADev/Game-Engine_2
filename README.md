# AetherRPG Maker

Eigenständige **3D-RPG-Maker-Software** mit eigener Engine, eigenem Editor und schlanker Runtime (`Game` / `Game.exe`).

> Keine Abhängigkeit von Unity, Unreal, Godot oder proprietärem RGSS-Quellcode.

## Komponenten

| Target | Beschreibung |
|--------|--------------|
| **aether_engine** | C++20-Engine (Core → Scene, Phys, Nav, Ruby, …) |
| **AetherEditor** | RPG-Maker-UI: Projekt · Karte · Datenbank · Events · Skripte · Testspiel · Export |
| **Game** | Runtime – lädt Projekt, Scripts, Plugins und startet das Spiel |

## Designziele

- Anfänger erstellen RPGs **ohne Programmierung** (Events, Datenbank, Objekt-Palette).
- Fortgeschrittene nutzen **Ruby** für Spiellogik und Plugins.
- **60 FPS**-Ziel auf älteren PCs (i3-4xxx / HD 4600, 8 GB RAM).
- Forward-Rendering, Frustum-Culling, LOD – **kein** Raytracing.
- **Keine** Unity-Style Component-/Collider-UI – Kollision & Navigation automatisch.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

**Optional für echtes Fenster + ImGui-GUI + OpenGL:**

```bash
sudo apt install libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/bin/AetherEditor --gui --project samples/demo_project
```

Ohne X11/GL baut das System automatisch **NullWindow + NullRenderer** (Headless/CI).

### Runtime

```bash
./build/bin/Game --project samples/demo_project --headless --max-frames 5
./build/bin/Game --project /path/MyGame
```

### Editor

```bash
./build/bin/AetherEditor --new /tmp/MyGame
./build/bin/AetherEditor --project /tmp/MyGame --testplay --headless
./build/bin/AetherEditor --gui --project /tmp/MyGame   # mit Display
```

**Editor-Tabs (verbindlich):** Projekt | Karte | Datenbank | Events | Skripte | Testspiel | Export

## Features (Stand)

| Bereich | Inhalt |
|---------|--------|
| Core | Logger, Time, Config, ThreadPool, EventBus, EngineContext |
| Window | Null + GLFW (optional) |
| Renderer | Null + OpenGL 3.3/GLAD (optional), Culling, LOD, GLSL 330 |
| Input | RPG-Actions (confirm/cancel/WASD…) |
| Audio | BGM/BGS/ME/SE – Null + **miniaudio** |
| Resources | VFS, Cache, JSON, **stb_image**, OBJ-Loader |
| Ruby | Stub-VM + Engine-API (mruby-Quellen unter `third_party/mruby-src` für späteren Host-Ruby-Build) |
| Scene | Objekte platzieren → **auto Kollision** |
| Physics | AABB move_and_collide, Trigger |
| Navigation | Grid-Bake aus Kollision, A*, NavAgent |
| Events | Interpreter (Message, Switch, Variable, Transfer, Script, …) |
| Database | actors/enemies/items/skills/system JSON |
| Plugins | `plugin.json` + `main.rb` |
| Export | Spielpaket mit Game-Binary |
| Editor UI | ImGui (wenn GL) / Headless-CLI |
| Tests | 13 automatisierte Tests |

## Vendored Third-Party

- `third_party/glad` – OpenGL 3.3 loader  
- `third_party/stb` – stb_image  
- `third_party/miniaudio` – Audio  
- `third_party/imgui` – Editor-UI  
- `third_party/mruby-src` – für künftigen mruby-Build (benötigt Host-Ruby)

FetchContent: nlohmann/json, glm, optional GLFW.

## Dokumentation

Siehe [`docs/architecture/`](docs/architecture/).

## Lizenz

MIT – siehe [LICENSE](LICENSE).
