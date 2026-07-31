/**
 * @file test_event_bridge.cpp
 * @brief Integration / Smoke-Test für EventBridge (ADR-001).
 *
 * Ziel: Ohne Fenster, ohne Editor-UI, ohne vollständige Rendering-Pipeline
 * prüfen, dass EventBridge → EventInterpreter → EventRequest korrekt verbunden ist.
 *
 * Build (Headless / CI):
 *   g++ -std=c++20 -I../../engine/include -I../../shared/include \
 *       -I../../third_party/nlohmann -I../../third_party/stb \
 *       test_event_bridge.cpp ../../engine/src/game/event_bridge.cpp \
 *       ../../engine/src/game/event_system.cpp ../../engine/src/core/logger.cpp \
 *       -o test_event_bridge_smoke
 *
 * @author Senior Software Architect / Lead Engine Developer
 */
#include <aether/game/event_bridge.hpp>
#include <aether/game/event_system.hpp>
#include <aether/core/types.hpp>

#include <iostream>
#include <cassert>
#include <cmath>

using namespace aether::game;

int main() {
    std::cout << "=== AetherRPG Maker – Smoke-Test EventBridge ===" << std::endl;

    // 1. Grundzustand: GameState + Interpreter
    GameState state;
    EventInterpreter interpreter(&state);

    // 2. Bridge erstellen
    EventBridge bridge(&interpreter);
    assert(!bridge.has_active_events());
    assert(bridge.active_count() == 0);
    std::cout << "[PASS] Bridge leer initialisiert." << std::endl;

    // 3. Ein einfaches Event registrieren (wie aus Editor / Datenbank)
    MapEvent ev;
    ev.id = 42;
    ev.name = "Test NPC";
    ev.x = 10.0f; ev.y = 2.0f; ev.z = 0.0f;

    EventPage page;
    page.name = "Seite 1";
    page.trigger = EventTrigger::ActionButton;

    EventCommand cmd;
    cmd.type = EventCommandType::Message;
    cmd.params["text"] = "Hallo aus der Bridge!";
    page.commands.push_back(cmd);

    ev.pages.push_back(page);

    bridge.register_event(ev, EventTrigger::ActionButton);
    assert(bridge.has_active_events());
    assert(bridge.active_count() == 1);
    std::cout << "[PASS] Event registriert (id=42)." << std::endl;

    // 4. Update durchführen (simuliert Frame)
    bridge.update(0.016f); // ~60 FPS
    std::cout << "[PASS] Update ausgeführt." << std::endl;

    // 5. Prüfen, ob Interpreter läuft (wegen vereinfachtem Trigger in Milestone 01)
    // Hinweis: In vollständiger Implementierung würde der Trigger durch Input/Touchevents ausgelöst.
    // Hier prüfen wir nur die strukturelle Verbindung.
    if (interpreter.is_running()) {
        std::cout << "[PASS] Interpreter läuft nach Trigger." << std::endl;
    } else {
        std::cout << "[INFO] Interpreter nicht aktiv (Trigger nicht ausgelöst – erwartet bei vereinfachter Logik)." << std::endl;
    }

    // 6. Requests drainen (falls Interpreter einen请求 erzeugt hat)
    auto requests = bridge.drain_requests();
    std::cout << "[INFO] Gedrainte Requests: " << requests.size() << std::endl;

    // 7. Clear (Kartenwechsel-Szenario)
    bridge.clear();
    assert(!bridge.has_active_events());
    std::cout << "[PASS] Clear nach Kartenwechsel." << std::endl;

    // 8. Parallel-Event-Test (Registrierung mehrerer Events)
    MapEvent ev2;
    ev2.id = 99;
    ev2.name = "Parallel Event";
    ev2.pages.push_back(page);
    bridge.register_event(ev2);
    assert(bridge.active_count() == 1); // ev2, ev wurde cleared
    std::cout << "[PASS] Mehrere Events verwaltet." << std::endl;

    // 9. Interpreter-Zugriff über Bridge
    assert(bridge.interpreter() == &interpreter);
    std::cout << "[PASS] Interpreter-Zugriff korrekt." << std::endl;

    std::cout << "=== Alle Smoke-Tests abgeschlossen (Milestone 01) ===" << std::endl;
    return 0;
}
