# Modul 2 – Ordnerstruktur

**Projekt:** AetherRPG Maker  
**Stand:** 0.1.0-dev

---

## 1. Übersicht

```
Game-Engine_2/
├── assets/                 # Engine- & Editor-Ressourcen (nicht Projekt-Assets)
├── build/                  # CMake Out-of-Source Build (gitignored)
├── cmake/                  # CMake-Module & Toolchains
├── docs/                   # Architektur-, API-, User-, Dev-Docs
├── editor/                 # Aether Editor (Executable)
├── engine/                 # Aether Engine (Library)
├── logs/                   # Laufzeit-Logs (gitignored Inhalt)
├── plugins/                # Mitgelieferte / Beispiel-Plugins
├── runtime/                # Game.exe Runtime
├── samples/                # Demo-Projekte
├── shared/                 # Gemeinsame Typen Editor ↔ Runtime
├── templates/              # Projekt-Vorlagen für „Neues Projekt“
├── tests/                  # Unit- & Integrationstests
├── third_party/            # Vendored Dependencies
├── tools/                  # Hilfsskripte, Validatoren
├── CMakeLists.txt          # Root Build
├── .gitignore
├── LICENSE
└── README.md
```

---

## 2. Engine (`engine/`)

```
engine/
├── include/aether/           # Öffentliche Headers
│   ├── aether.hpp            # Umbrella-Header
│   ├── core/                 # Logger, Types, Time, Config, ThreadPool, …
│   ├── window/               # Window, GlContext
│   ├── render/               # Renderer, Mesh, Shader, Camera, …
│   ├── input/                # Keyboard, Mouse, Gamepad, Actions
│   ├── audio/                # AudioEngine, BGM/BGS/ME/SE
│   ├── res/                  # ResourceManager, Loader
│   ├── scene/                # Scene, Entity, SceneManager
│   ├── anim/                 # Animation, Animator
│   ├── phys/                 # Collision world (automatisch)
│   ├── nav/                  # Pathfinding
│   ├── ruby/                 # Ruby VM + Bindings
│   ├── plugin/               # PluginLoader
│   └── game/                 # Player, NPC, Enemy, Quest, …
├── src/                      # Implementierungen (gleiche Unterordner)
└── shaders/                  # GLSL-Quellen (ggf. nach assets kopiert)
```

**Regel:** Alles unter `include/aether/**` ist die stabile C++-API.  
Interne Details bleiben in `src/` oder `detail/`-Headern.

---

## 3. Editor (`editor/`)

```
editor/
├── include/aether/editor/
├── src/
│   ├── app/                  # Application, MainWindow
│   ├── project/              # ProjectModel, New/Open/Save
│   ├── map/                  # MapEditor, ObjectPalette, Drag&Drop
│   ├── database/             # DB-Tabs (Actors, Items, …)
│   ├── events/               # Visueller Event-Editor
│   ├── scripts/              # Ruby-IDE, Debugger, Hot-Reload
│   ├── testplay/             # Testspiel-Launcher
│   ├── export/               # Export-Wizard
│   └── ui/                   # Widgets, Themes, Dialoge
└── resources/
    ├── icons/
    ├── themes/
    └── i18n/
```

**UI-Philosophie:** Menüs und Tabs entsprechen den Anwender-Konzepten  
(Projekt, Karte, Datenbank, Events, Skripte, Testspiel, Export) –  
**keine** Engine-Component-Hierarchie.

---

## 4. Runtime (`runtime/`)

```
runtime/
├── include/aether/runtime/
└── src/                      # main.cpp, bootstrap, arg-parser
```

Erzeugt das Executable **Game** (`Game.exe` unter Windows).  
Lädt `project.json`, Assets, Skripte, Plugins und startet die Titelszene.

---

## 5. Shared (`shared/`)

Gemeinsame, serialisierbare Datenstrukturen und Pfad-Helfer, die sowohl  
Editor als auch Runtime benötigen (z. B. `ProjectDescriptor`, `MapData`,  
`DatabaseSchema`-Versionen) – **ohne** UI und **ohne** schwere Engine-Deps.

---

## 6. Assets vs. Projekt-Assets

| Pfad | Inhalt |
|------|--------|
| `assets/engine/` | Built-in Shaders, Fallback-Texturen, Default-Fonts |
| `assets/editor/` | Icons, Themes, Layout-Defaults |
| `templates/empty_project/` | Vorlage für neue Spiele |
| `samples/demo_project/` | Vollständiges Beispiel-RPG |
| `<user-project>/` | Vom Anwender erstellte Inhalte |

### Projektlayout (Spieler-/Autor-Projekt)

```
MyGame/
├── project.json
├── data/                 # actors.json, items.json, …
├── maps/                 # map001.json, …
├── graphics/
│   ├── characters/
│   ├── tilesets/
│   ├── battlers/
│   ├── pictures/
│   ├── titles/
│   └── system/
├── audio/
│   ├── bgm/
│   ├── bgs/
│   ├── me/
│   └── se/
├── scripts/              # main.rb + weitere
└── plugins/              # optionale Plugins
```

---

## 7. Plugins

```
plugins/example_plugin/
├── plugin.json
├── main.rb
└── assets/
```

Zur Laufzeit werden Plugins aus dem Projektordner `plugins/` geladen.  
Mitgelieferte Beispiele liegen unter Repo-`plugins/`.

---

## 8. Tests

```
tests/
├── unit/           # Core, Math, Serialization, …
├── integration/    # Engine-Boot, Resource-Load, Ruby-Smoke
└── fixtures/       # Minimale Test-Assets / JSON
```

---

## 9. Docs

```
docs/
├── architecture/   # Modul-Docs (dieses Dokument)
├── api/            # C++ & Ruby API-Referenz
├── user/           # Handbuch für Spiele-Autoren
└── dev/            # Contributor-Guide, Coding-Standards
```

---

## 10. third_party-Politik

- Bevorzugt: CMake `FetchContent` oder Git-Submodules.
- Vendoring nur wenn nötig (Offline-Builds, Patches).
- Lizenzen in `third_party/NOTICE` sammeln.

---

## 11. Include-Pfade (Ziel)

```
#include <aether/core/logger.hpp>
#include <aether/render/renderer.hpp>
#include <aether/editor/main_window.hpp>   // nur Editor-Target
#include <aether/runtime/bootstrap.hpp>    # nur Runtime-Target
#include <aether/shared/project_descriptor.hpp>
```

---

*Nächstes Modul: Build-System*
