# ADR 001 – Event-Engine-Bridge (EventBridge)

**Status:** Accepted  
**Datum:** 2026-07-31  
**Autor:** Senior Software Architect / Lead Engine Developer  
**Projekt:** AetherRPG Maker (C++20, eigene Engine, Editor, Runtime)

---

## 1. Kontext

Das RPG-Maker-Eventsystem (`EventInterpreter`, `EventCommand`, `MapEvent`) ist in `engine/src/game/event_system.cpp` implementiert. Der Editor (`editor/src/ui/editor_app.cpp`) erzeugt `MapEvent`-Objekte und zeigt sie in `draw_events_tab()`. Die Runtime (`runtime/src/bootstrap.cpp`) initialisiert einen `EventInterpreter` und führt ihn im Main-Loop aus.

**Problem:** Es gibt keine explizite, testbare Schicht zwischen Editor-Event-Daten und Runtime-Interpreter. Der Editor speichert Events in `map_scene_->objects()`; die Runtime liest sie über `load_map_into()` und erstellt einen `EventRunner`. Diese indirekte Kopplung verhindert:

- Klare Unit-Tests des Event-Flusses.
- Sauberes Deaktivieren von Events bei Kartenwechsel.
- Erweiterungen wie Parallel-Events oder Trigger-Conditions ohne Editor-UI-Änderungen.

---

## 2. Entscheidung

Wir führen eine **EventBridge**-Schicht ein (Adapter / Facade-Pattern).

### 2.1 Schnittstelle (siehe Milestone-Dok)

```cpp
class EventBridge {
    void register_event(const MapEvent&, EventTrigger);
    void clear();
    void update(float dt);
    std::vector<EventRequest> drain_requests();
};
```

### 2.2 Implementierungsdetails

- **Speicher:** `std::unordered_map<EntityId, std::unique_ptr<MapEvent>>` (O(1) Zugriff, keine Duplikate).
- **Trigger-Logik:** Pro Frame prüft `update()` jede registrierte Event-ID auf `EventTrigger`. Wenn `ActionButton` und Interaktion erkannt → `interpreter->start()`.
- **Parallel-Events:** Mehrere Events können gleichzeitig aktiv sein (`autorun`, `parallel`). Die Bridge startet den Interpreter nicht neu, solange einer läuft, sondern queue-t zusätzliche Requests.
- **Lifecycle:** `clear()` wird von `RuntimeState` aufgerufen bei Kartenwechsel (nach `load_map_into()`).

---

## 3. Konsequenzen

### Positiv

- **Testbarkeit:** `test_event_bridge.cpp` kann Bridge + Interpreter ohne Fenster / ohne Editor-UI testen.
- **Entkopplung:** Editor muss nicht mehr direkt in `EventInterpreter` schreiben; nur `register_event()`.
- **Erweiterbarkeit:** Neue Trigger-Typen (`EventTouch`, `Autorun`) werden nur in Bridge + Editor-UI ergänzt, nicht in Runtime-Core.

### Negativ / Risiken

- **Overhead:** Extra Zwischenschicht (minimale Allokation, akzeptabel bei n < 100 aktive Events).
- **Synchronisation:** Bridge und Interpreter laufen im gleichen Thread (Main-Loop), daher kein Lock-Overhead – aber keine Parallelisierung möglich.

---

## 4. Alternative Erwägungen (abgelehnt)

| Alternative | Begründung für Ablehnung |
|-------------|--------------------------|
| Direkte Kopplung Editor → Interpreter | Keine Testbarkeit, kein Kartenwechsel-Clear, Editor-UI-Dependencies im Engine-Core |
| Observer-Pattern mit `std::function`-Callbacks überall | Zu komplex für RPG-Maker-Eventzähler (max. 4096 Events); Bridge zentralisiert Logik |
| Komponenten-System (Unity-Style) | Explizit laut README abgelehnt („Keine Unity-Style Component-/Collider-UI“) |

---

## 5. Bezüge

- Milestone-Dok: `docs/dev/milestone_01_event_engine.md`
- Interface: `engine/include/aether/game/event_bridge.hpp`
- Implementierung: `engine/src/game/event_bridge.cpp`
- Integrationstest: `tests/integration/test_event_bridge.cpp`
- Runtime-Init: `runtime/src/bootstrap.cpp` (Zeile ~698, Plugin-Loader + Ruby-VM – Bridge wird analog initialisiert)

---

*Genehmigt als Lead-Architect-Entscheidung für Milestone 01. Keine Rücknahme geplant, solange das 60-FPS-Ziel und die Headless-CI-Anforderungen eingehalten werden.*
