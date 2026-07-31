# Milestone 01 – Event-Engine-Integration & Runtime-Bridge

**Status:** DESIGN → IMPLEMENTIERUNG  
**Lead Architect:** Senior Software Architect / Lead Engine Developer  
**Ziel:** Standardisierte Kommunikation zwischen Editor (MapEvent / Datenbank), Engine (EventInterpreter / GameState) und Runtime (Game.exe / Plugin-System).

---

## 1. Kontext & Problemstellung

Das Repository `AetherRPG Maker` besitzt bereits:

- `engine/src/game/event_system.cpp` – Datenmodell + Interpreter (`EventInterpreter`, `GameState`, `EventRequest`)
- `editor/src/ui/editor_app.cpp` – Visueller Event-Editor (`draw_events_tab()`)
- `runtime/src/bootstrap.cpp` – Runtime-Initialisierung (`RuntimeState`, Plugin-Loader, Ruby-VM)

**Fehlend / nicht standardisiert:**

1. Eine explizite **Brücke** zwischen Editor-Events (`MapEvent`, platziert auf `map_scene_`) und Runtime-Event-Lauf (`EventInterpreter`).
2. Ein **Lifecycle-Management** für Events (Registrierung, Trigger, Deaktivierung bei Kartenwechsel, Parallel-Event-Handling).
3. Ein **Smoke-Test / Integrationstest**, der sicherstellt, dass `EventBridge` → `EventInterpreter` → `RuntimeState` korrekt verbunden sind.

---

## 2. Architektur-Entscheidung (ADR-001)

Siehe [`adr_001_event_bridge.md`](adr_001_event_bridge.md).

**Kernaussagen:**

- **Pattern:** Adapter + Observer (Editor veröffentlicht `MapEvent`, Runtime subscribt via `EventBridge`).
- **Sprache:** C++20 (`std::function`, `std::optional`, `std::views` wo sinnvoll, `noexcept`-Contracte).
- **Abhängigkeit:** Nur `aether::game` (kein Editor-UI-Dependency im Engine-Core).
- **Threading:** Single-Threaded im Main-Loop (`update()` wird von `RuntimeState` aufgerufen, nicht parallel).

---

## 3. Komponenten

| Komponente | Datei | Verantwortung |
|------------|-------|---------------|
| **EventBridge (Interface)** | `engine/include/aether/game/event_bridge.hpp` | API-Contract: registrieren, triggern, updaten, requests auslesen |
| **EventBridge (Impl)** | `engine/src/game/event_bridge.cpp` | Lebenszyklus, Parallel-Events, Übergabe an `EventInterpreter` |
| **Integrationstest** | `tests/integration/test_event_bridge.cpp` | Smoke-Test: Event registrieren → trigger → Interpreter läuft → Request erzeugt |
| **Runtime-Init** | `runtime/src/bootstrap.cpp` (Patch-Doku) | `EventBridge` in `RuntimeState` initialisieren |

---

## 4. Schnittstelle (API-Contract)

```cpp
namespace aether::game {

class EventBridge {
public:
    explicit EventBridge(EventInterpreter* interpreter);

    // Registriert ein MapEvent (aus Editor / Datenbank) für diese Karte.
    void register_event(const MapEvent& ev, EventTrigger default_trigger = EventTrigger::ActionButton);

    // Entfernt alle Events dieser Karte (bei Kartenwechsel).
    void clear();

    // Prüft alle registrierten Events gegen Trigger-Bedingungen.
    // Ruft eventuell `interpreter->start()` auf.
    void update(float dt);

    // Liefert pending Requests an RuntimeState (Choice, Battle, Shop, …).
    [[nodiscard]] std::vector<EventRequest> drain_requests();

    [[nodiscard]] bool has_active_events() const noexcept;
    [[nodiscard]] usize active_count() const noexcept;
};

} // namespace aether::game
```

---

## 5. Datenfluss

```
Editor (draw_events_tab)
  │  MapEvent (JSON / Datenbank)
  ▼
Engine-Core (EventBridge::register_event)
  │  Speichert in interne Map<EventId, MapEvent>
  ▼
Runtime (bootstrap.cpp → RuntimeState::event_bridge)
  │  Each frame: EventBridge::update(dt)
  │  • Prüfe Trigger (ActionButton, Touch, Autorun, Parallel)
  │  • Falls Trigger erfüllt: interpreter->start(event.commands)
  ▼
EventInterpreter (update())
  │  • Führt Befehle aus (Message, Choice, Transfer, …)
  │  • Erzeugt EventRequest (Choice, Battle, Shop, …)
  ▼
RuntimeState::handle_event_requests()
  │  • Öffnet UI / startet Battle / zeigt Shop
  │  • Setzt interpreter->resume_choice(index) oder interpreter->clear_pending()
  ▼
Zurück zu RuntimeLoop (60 FPS-Ziel)
```

---

## 6. Designziele (aus README übernommen)

- **Kein Raytracing, kein Unity-Style Component-UI** – reine Forward-Engine.
- **Anfänger ohne Programmierung** – Events über Editor definierbar, Bridge übernimmt Lauf.
- **Fortgeschrittene via Ruby** – `EventInterpreter::set_script_handler()` bleibt erhalten; Bridge ruft nur auf.
- **60 FPS auf i3-4xxx / HD 4600** – Bridge ist O(n) pro Frame (n = aktive Events), keine schweren Allokationen.

---

## 7. Nächste Schritte (nach Milestone 01)

| Nr. | Aufgabe | Ziel |
|-----|---------|------|
| 1.1 | Plugin-API-Dok + Sandbox für Ruby-Plugins | `docs/dev/plugin_api.md` |
| 1.2 | Editor-Undo/Redo für Event-Seiten (`undo_stack.cpp` erweitern) | Datenintegrität beim Editieren |
| 1.3 | Game.exe Release-Packaging (Linux + Windows Archive) | `release.yml` aktivieren |
| 2.0 | Frustum-Culling + LOD für Map-Meshes | Rendering-Pipeline optimieren |
| 3.0 | Navigation / Pathfinding-Integration in Runtime | Automatische Kollision & Wegfindung |

---

*Autorenhinweis: Diese Datei wurde als Senior-Architect-Deliverable erstellt, basierend auf dem bestehenden CMake/C++20-Stack, der NullBackends für Headless-CI und offenen GLFW/OpenGL/Render-Modulen.*
