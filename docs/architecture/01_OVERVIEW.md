# Modul 1 – Gesamtarchitektur

**Projekt:** AetherRPG Maker  
**Codename:** Aether Engine  
**Version:** 0.1.0-dev  
**Zielplattform:** Windows 64-bit (primär), Linux 64-bit (Entwicklung)

---

## 1. Vision

AetherRPG Maker ist eine eigenständige 3D-RPG-Maker-Software mit:

| Komponente | Beschreibung |
|------------|--------------|
| **Aether Engine** | Eigenständige C++20-Engine (Shared/Static Library) |
| **Aether Editor** | Visueller Editor im klassischen RPG-Maker-Stil |
| **Game.exe** | Schlanke Runtime ohne Editor-UI |
| **Ruby VM** | Eingebettete Ruby-Schnittstelle für Spiellogik |

**Keine** Abhängigkeit von Unity, Unreal, Godot oder proprietärem RGSS-Code.

---

## 2. Designprinzipien

1. **Einfachheit vor Technik** – Der Anwender sieht Projekte, Karten, Datenbank, Events – keine Components, Collider oder Rigidbodies.
2. **Automatik wo möglich** – Kollision, Navigation und Standardverhalten werden von der Engine abgeleitet.
3. **Modularität** – Jedes Subsystem hat eine klare Schnittstelle (SOLID, RAII).
4. **Performance-Budget** – 60 FPS auf Intel i3-4xxx + HD 4600, 8 GB RAM.
5. **Erweiterbarkeit** – Plugins (`plugin.json` + `main.rb` + `assets/`) und Ruby-Skripte.
6. **Keine globalen Variablen** – Zugriff über `Engine::Context` / Service-Locator mit expliziter Lebensdauer.

---

## 3. High-Level-Schichten

```
┌─────────────────────────────────────────────────────────────┐
│  Aether Editor          │  Game.exe (Runtime)               │
│  (Qt/Custom UI)         │  (Headless UI, nur Spiel)         │
├─────────────────────────┴───────────────────────────────────┤
│  Application Layer                                          │
│  Project · Map · Database · Events · Scripts · Export       │
├─────────────────────────────────────────────────────────────┤
│  Game Framework                                             │
│  SceneManager · Player · NPC · Enemy · Quest · Dialogue     │
│  Inventory · Weather · Camera · Party                       │
├─────────────────────────────────────────────────────────────┤
│  Scripting Bridge (Ruby C-API)                              │
│  Graphics · Audio · Input · Map · …                         │
├─────────────────────────────────────────────────────────────┤
│  Aether Engine Core                                         │
│  Core · Window · Renderer · Input · Audio · Resources       │
│  Scene · Animation · Physics · Navigation · Plugins         │
├─────────────────────────────────────────────────────────────┤
│  Platform / Third-Party                                     │
│  GLFW · OpenGL 3.3 / GLAD · OpenAL-Soft · stb · nlohmann    │
│  assimp · miniaudio · mruby/CRuby                           │
└─────────────────────────────────────────────────────────────┘
```

---

## 4. Prozessmodell

### 4.1 Editor-Prozess

```
main()
  → Engine::Context::create(EditorConfig)
  → Editor::Application::run()
       ├─ UI-Thread: Fenster, Widgets, Drag&Drop
       ├─ Engine-Thread: Update/Render (optional shared)
       └─ Ruby-Thread: Script-Debugger / Hot-Reload (kooperativ)
```

### 4.2 Runtime-Prozess (Game.exe)

```
main()
  → lade project.json
  → Engine::Context::create(RuntimeConfig)
  → lade Plugins + Ruby-Skripte
  → SceneManager::boot(title_scene | map_scene)
  → while running: fixed_update → update → render
  → shutdown
```

---

## 5. Subsysteme (Verantwortlichkeiten)

| Subsystem | Namespace | Aufgabe |
|-----------|-----------|---------|
| **Core** | `aether::core` | Logger, Assert, Types, Time, Thread-Pool, Config, Events (C++) |
| **Window** | `aether::window` | Fenster, Kontext, VSync, Fullscreen |
| **Renderer** | `aether::render` | OpenGL 3.3, Mesh, Material, Shader, Camera, Frustum-Culling, LOD |
| **Input** | `aether::input` | Tastatur, Maus, Gamepad, Action-Mapping |
| **Audio** | `aether::audio` | BGM, BGS, ME, SE (kanäle analog klassischer RPG Maker) |
| **Resources** | `aether::res` | Asset-Cache, glTF/FBX/OBJ, Texturen, Audio, JSON |
| **Scene** | `aether::scene` | Szenengraph, Entities (nicht ECS-Components für User!) |
| **Animation** | `aether::anim` | Skeletal + simple Tween |
| **Physics** | `aether::phys` | Einfache AABB/Capsule-Kollision (automatisch) |
| **Navigation** | `aether::nav` | Grid-/NavMesh-Pfadfindung für NPCs |
| **Ruby** | `aether::ruby` | VM-Einbettung, API-Bindings, Hot-Reload |
| **Plugins** | `aether::plugin` | Laden von `plugin.json` + Ruby-Entry |
| **Game** | `aether::game` | Player, NPC, Enemy, Quest, Inventory, Dialogue, Weather |
| **Editor** | `aether::editor` | Projekt, Karte, DB, Events, Script-IDE, Testspiel, Export |
| **Runtime** | `aether::runtime` | Game.exe Bootstrap |

---

## 6. Datenfluss (vereinfacht)

```
project/
  project.json          → ProjectDescriptor
  data/
    actors.json         → Database
    enemies.json
    items.json
    skills.json
    ...
  maps/
    map001.json         → MapData (Tiles/Objects/Events)
  graphics/             → Textures, Models
  audio/                → BGM, SE, …
  scripts/              → main.rb, …
  plugins/              → plugin folders
```

**JSON** ist das kanonische Projektdatenformat.  
Binary-Caches (`.aether_cache`) können zur Laufzeit erzeugt werden.

---

## 7. Threading-Modell

| Thread | Arbeit |
|--------|--------|
| **Main** | Fenster-Events, Editor-UI, Frame-Orchestrierung |
| **Render** | (optional) Command-Buffer ausführen – Phase 1: Main-Thread GL |
| **Audio** | Mixer-Callback (Bibliothek-intern) |
| **Workers** | Asset-Loading, Nav-Bake, Thumbnail-Gen (Thread-Pool) |
| **Ruby** | Kooperativ auf Main oder dediziert mit GIL-Achtsamkeit |

Phase 1 nutzt **Main-Thread OpenGL** + **Worker-Pool für I/O**, um Komplexität und HD-4600-Kompatibilität zu halten.

---

## 8. Rendering-Pipeline (Ziel)

1. Scene collect visible (Frustum Culling)
2. LOD-Auswahl (Distanz)
3. Opaque pass (Forward, sortiert nach Material)
4. Transparent pass (back-to-front)
5. UI / Dialogue overlay
6. Optional: simple post (color grade, vignette) – **kein** Raytracing

**API:** OpenGL 3.3 Core Profile (breite Treiberunterstützung inkl. Intel HD 4600).

---

## 9. Ruby-API (konzeptionell)

```ruby
# Nur Spiellogik – keine Engine-Internals
Graphics.frame_rate = 60
Audio.bgm_play("Theme1", 80, 100)
Input.press?(:C)

SceneManager.goto(Scene_Map)
Player.transfer(map_id, x, y, z, direction)
NPC.find("elder").say("Willkommen, Held!")
Weather.set(:rain, power: 5)
Inventory.gain(:potion, 3)
Quest.start(:main_001)
Dialogue.start("intro_01")
Map.tint(r, g, b, frames)
```

Die API orientiert sich **konzeptionell** an klassischen RPG-Script-Layern, ist aber **vollständig neu** implementiert.

---

## 10. Editor-Bedienkonzept

Der Editor zeigt **nur**:

- **Projekt** – Neu / Öffnen / Speichern / Einstellungen (Spiel-Titel, Auflösung)
- **Karte** – 3D-Ansicht, Drag&Drop von Objekten, Event-Platzierung
- **Datenbank** – Helden, Gegner, Items, Skills, Klassen, Animationen, System
- **Events** – Visueller Event-Editor (keine Code-Pflicht)
- **Skripte** – Ruby-IDE mit Highlighting, Autocomplete, Debugger
- **Testspiel** – Startet Runtime im Debug-Modus
- **Export** – Erzeugt verteilbares Game-Paket + Game.exe

**Nicht sichtbar:** Component-Listen, Collider-Inspector, Rigidbody, Shader-Graph, Raw-Transform-Gizmos als Pflicht.

Objekte aus der Objekt-Palette → Drop auf Karte → Engine setzt Kollision/Nav automatisch anhand von Objekttyp und Mesh-Bounds.

---

## 11. Plugin-Format

```
plugins/my_plugin/
  plugin.json      # name, version, author, entry, dependencies
  main.rb          # Entry-Script
  assets/          # optionale Ressourcen
```

```json
{
  "name": "MyPlugin",
  "version": "1.0.0",
  "author": "Author",
  "entry": "main.rb",
  "dependencies": [],
  "api_version": 1
}
```

---

## 12. Build-Konfigurationen

| Config | Makros | Zweck |
|--------|--------|-------|
| **Debug** | `AETHER_DEBUG=1`, Assertions, Logging verbose | Entwicklung |
| **Release** | `NDEBUG`, Optimierungen `-O2`/`/O2` | Verteilung |
| **RelWithDebInfo** | Optimiert + Symbole | Profiling |

Zielarchitektur: **x86_64** exclusively.

---

## 13. Abhängigkeiten (Third-Party, freizügig lizenziert)

| Lib | Nutzung | Lizenz |
|-----|---------|--------|
| GLFW 3.4 | Fenster / Input | zlib |
| GLAD | OpenGL Loader | MIT |
| glm | Mathematik | Happy Bunny / MIT |
| nlohmann/json | JSON | MIT |
| stb_image | Texturen | Public Domain |
| assimp | Model-Import | BSD |
| miniaudio oder OpenAL-Soft | Audio | Public Domain / LGPL |
| mruby oder CRuby | Scripting | MIT / BSD-ähnlich |
| Dear ImGui *(nur Editor-Debug)* | optional Dev-Tools | MIT |
| Qt 6 *oder* custom UI | Editor-UI | LGPL/Commercial bzw. eigen |

**Hinweis:** Editor-UI kann zunächst mit einer schlanken Eigen-UI + ImGui-Prototype starten; Produktions-UI kann auf Qt wechseln, ohne die Engine-API zu ändern.

---

## 14. Fehlerbehandlung & Logging

```
[Timestamp] [Thread] [Level] [Subsystem] Message
```

Level: `Trace`, `Debug`, `Info`, `Warn`, `Error`, `Fatal`  
Sinks: Konsole, Datei (`logs/aether_YYYYMMDD.log`), optional Editor-Panel.

---

## 15. Modul-Roadmap (Arbeitsreihenfolge)

| # | Modul | Status |
|---|-------|--------|
| 1 | Gesamtarchitektur | ✅ |
| 2 | Ordnerstruktur | ✅ |
| 3 | Build-System | ✅ |
| 4 | Core | ✅ |
| 5 | Fenster | ✅ (Null + GLFW optional) |
| 6 | Renderer | ✅ (Null + OpenGL, Culling/LOD) |
| 7 | Input | ✅ (+ Gamepad) |
| 8 | Audio | ✅ (Null + MiniAudio) |
| 9 | Ressourcenverwaltung | ✅ (glTF/GLB, FBX, OBJ, PNG/JPG, WAV/OGG/MP3) |
| 10 | Ruby-Einbindung | ✅ (Stub + mruby; echte Host-Bindings) |
| 11 | Editor | ✅ ImGui-GUI (Projekt/Karte/Datenbank/Events/Skripte/Testspiel/Export) |
| 12 | Runtime | ✅ (Game.exe: Titel → Map → Menü/Dialog/Kampf/Save) |
| 13 | Eventsystem | ✅ Interpreter + JSON |
| 14 | Datenbank | ✅ JSON |
| 15 | Export | ✅ Dateisystem-Paket |
| 16 | Plugins | ✅ Loader |
| 17 | Tests | ✅ 22 Unit/Smoke |
| 18 | Physik/Navigation/Szene | ✅ (AABB, A*, NavGrid) |
| 19 | CI & mruby | ✅ (Linux/Windows/macOS-Workflows) |
| 20 | Gameplay-Runtime | ✅ (HUD, Fade, Testspiel) |
| 21 | Szenen/Save/Inventar/Shop | ✅ |
| 22 | Kampf/Quest/HUD | ✅ |
| 23 | glTF/Script-IDE/Undo | ✅ |
| 24 | FBX-Import | ✅ eigener Reader (ASCII + binär 7.x) |
| 25 | Ruby-Host-API | ✅ echte Engine-Bindings |
| 26 | Textur-Pipeline | ✅ PNG/JPG → GPU (stb, GL-Upload, Shader-Sampler) |
| 27 | Character-Sprites & GameOver | ✅ Billboard-Quads, Pixel-Art-Demo, Game-Over-Screen |

---

## 16. Namenskonventionen

| Element | Konvention | Beispiel |
|---------|------------|----------|
| Namespaces | `snake` lower | `aether::core` |
| Klassen | `PascalCase` | `ResourceManager` |
| Funktionen | `snake_case` | `load_texture` |
| Member | `snake_case_` | `frame_count_` |
| Konstanten | `kPascal` oder `UPPER_SNAKE` | `kMaxLights` |
| Dateien | `snake_case.hpp/.cpp` | `logger.hpp` |
| Makros | `AETHER_…` | `AETHER_ASSERT` |

---

## 17. Qualitätsanforderungen

- C++20
- RAII überall
- `std::unique_ptr` / `std::shared_ptr` bewusst einsetzen
- Keine rohen `new`/`delete` ohne Owner
- Doxygen-Kommentare an öffentlichen APIs
- Header sauber halten (`#pragma once`, Forward-Decls)
- Unit-Tests für Core-Logik (Catch2 oder GoogleTest)

---

*Nächstes Modul: Ordnerstruktur*
