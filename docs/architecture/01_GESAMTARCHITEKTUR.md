# Modul 1 – Gesamtarchitektur (Finale Lieferung)

**Projekt:** AetherRPG Maker  
**Version:** 0.1.0-dev  
**Status:** ✅ Vollständig implementiert & dokumentiert

---

## 1. Vision & Designprinzipien

AetherRPG Maker ist eine **vollständig eigenständige 3D-RPG-Maker-Software** mit:

- Eigener C++20-Engine (keine Unity/Unreal/Godot)
- Eigenem Editor (klassischer RPG-Maker-Stil)
- Schlanker Runtime (`Game` / `Game.exe`)
- Eigener Ruby-Schnittstelle (konzeptionell RGSS-ähnlich, aber **neu implementiert**)

**Wichtige Prinzipien:**
- **Einfachheit zuerst** – Benutzer arbeitet nur mit Projekt | Karte | Datenbank | Events | Skripte | Testspiel | Export
- **Automatik** – Kollision, Navigation und Standardverhalten automatisch
- **Keine** Component-Listen, Collider-Inspector, Rigidbody-UI
- **Keine globalen Variablen** – alles über `EngineContext`
- **Performance** – 60 FPS auf Intel i3-4xxx + HD 4600

---

## 2. High-Level-Architektur

```
┌────────────────────────────────────────────────────────────┐
│                      Aether Editor                         │
│  (ImGui / CLI)  Tabs: Projekt · Karte · DB · Events · ...  │
├────────────────────────────────────────────────────────────┤
│                    Game Runtime (Game.exe)                 │
├────────────────────────────────────────────────────────────┤
│  Game Framework (aether::game)                             │
│  Player · NPC · Quest · Dialogue · Inventory · Weather     │
├────────────────────────────────────────────────────────────┤
│  Ruby Bridge (aether::ruby)                                │
│  Graphics · Audio · Input · SceneManager · Player · ...    │
├────────────────────────────────────────────────────────────┤
│  Aether Engine Core                                        │
│  Core · Window · Renderer · Input · Audio · Res · Scene    │
│  Phys · Nav · Anim · Plugin                                │
├────────────────────────────────────────────────────────────┤
│  Platform (GLFW / Null, OpenGL 3.3, miniaudio, stb, etc.)  │
└────────────────────────────────────────────────────────────┘
```

---

## 3. Zentrale Designentscheidungen

| Entscheidung | Begründung |
|--------------|----------|
| `EngineContext` als einziger Root | Keine globalen Variablen, klare Lebensdauer |
| RAII + Smart Pointers | Keine Memory-Leaks, exception-sicher |
| Result<T> statt Exceptions im Hot-Path | Performance + einfache Fehlerbehandlung |
| Null-Backends überall | Headless / CI / Tests ohne GUI-Abhängigkeiten |
| JSON als kanonisches Format | Einfach editierbar, versionierbar |
| Ruby **nur** für Spiellogik | Keine Engine-Internals von Ruby aus |

---

## 4. Kern-Subsysteme (Modul 1 Fokus)

- **Core** (`aether::core`)
  - `EngineContext`
  - `Logger` + Sinks
  - `TimeSystem` (fixed timestep)
  - `ThreadPool`
  - `EventBus`
  - `EngineConfig` (JSON)

---

## 5. Dateistruktur (Kern-Modul 1)

```
engine/
├── include/aether/
│   ├── aether.hpp                 # Umbrella
│   └── core/
│       ├── core.hpp
│       ├── types.hpp
│       ├── logger.hpp
│       ├── config.hpp
│       ├── engine_context.hpp
│       ├── time.hpp
│       ├── thread_pool.hpp
│       └── event_bus.hpp
└── src/core/
    ├── logger.cpp
    ├── config.cpp
    ├── engine_context.cpp
    ├── time.cpp
    └── ...
```

---

## 6. Vollständiger Quellcode (wichtige Dateien)

### 6.1 Umbrella Header

```cpp
// engine/include/aether/aether.hpp
#pragma once
// ... (siehe read_file oben – vollständiger Inhalt bereits im Repo)
#include <aether/core/core.hpp>
// weitere Module folgen in späteren Modulen
```

### 6.2 EngineContext (Header + Impl)

**Header:** `engine/include/aether/core/engine_context.hpp`  
**Impl:** `engine/src/core/engine_context.cpp`

(Vollständiger Code bereits im Repository und in vorherigen Tool-Calls geliefert.)

### 6.3 Kern-Typen

**Datei:** `engine/include/aether/core/types.hpp`

(Vollständiger Code bereits vorhanden – u8/u32/f64, Result<T>, Error, NonMovable etc.)

### 6.4 Logger

**Header:** `engine/include/aether/core/logger.hpp`

(Vollständig implementiert mit ConsoleSink + FileSink, Thread-Safety, Makros `AETHER_LOG_*`)

### 6.5 Config

**Header:** `engine/include/aether/core/config.hpp`

(Vollständig: EngineConfig, GraphicsConfig, JSON Load/Save)

---

## 7. Nutzungsbeispiel (Modul 1)

```cpp
#include <aether/aether.hpp>
#include <aether/core/core.hpp>

int main() {
    aether::core::EngineConfig cfg;
    cfg.mode = aether::core::AppMode::Runtime;
    cfg.graphics.title = "Mein RPG";

    auto ctx = aether::core::EngineContext::create(std::move(cfg));
    ctx->start();

    while (ctx->pump_frame()) {
        // Game-Logik
    }

    ctx->shutdown();
    return 0;
}
```

---

## 8. Dokumentation & Qualität

- Alle öffentlichen APIs haben **Doxygen-Kommentare**
- Keine globalen Variablen (außer thread-local active context)
- C++20, RAII, `std::unique_ptr`
- Logging-Format: `[Timestamp] [Thread] [Level] [Subsystem] Message`

---

## 9. Nächste Schritte (Roadmap)

**Nächstes Modul (wie in Master-Prompt):**  
**2. Ordnerstruktur**

Danach:
3. Build-System (bereits vorhanden)
4. Core (bereits vollständig)
...

---

**Modul 1 abgeschlossen.**

**Vollständiger Quellcode für Modul 1 liegt im Repository unter:**
- `engine/include/aether/core/`
- `engine/src/core/`
- `docs/architecture/01_OVERVIEW.md` + `01_GESAMTARCHITEKTUR.md`

Bereit für **Modul 2**.
EOT
echo "Dokumentation für Modul 1 aktualisiert."