/**
 * @file event_bridge.hpp
 * @brief EventBridge – Standardisierte Brücke zwischen Editor-Events und Runtime-EventInterpreter.
 *
 * Architektur (ADR-001):
 * - Editor registriert MapEvent über register_event().
 * - Runtime ruft update(dt) im Main-Loop auf.
 * - Interpreter produziert EventRequest; RuntimeState konsumiert via drain_requests().
 *
 * @author Senior Software Architect / Lead Engine Developer
 */
#pragma once

#include <aether/game/event_system.hpp>

#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>

namespace aether::game {

/**
 * @brief Brücke zwischen Editor-Karten-Events und Runtime-EventInterpreter.
 *
 * Design-Entscheidungen (ADR-001):
 * - Single-Threaded (Main-Loop).
 * - Keine Editor-UI-Abhängigkeit im Engine-Core.
 * - O(n) Update mit n = aktive Events (max. 4096, typisch < 100).
 */
class EventBridge {
public:
    explicit EventBridge(EventInterpreter* interpreter) noexcept;

    EventBridge(const EventBridge&) = delete;
    EventBridge& operator=(const EventBridge&) = delete;
    EventBridge(EventBridge&&) noexcept = default;
    EventBridge& operator=(EventBridge&&) noexcept = default;
    ~EventBridge() = default;

    /**
     * @brief Registriert ein MapEvent (aus Editor / Datenbank) für die aktive Karte.
     * @param ev Das Event-Objekt (enthält id, name, pages, trigger).
     * @param default_trigger Fallback-Trigger, falls Seite keine explizite Bedingung hat.
     */
    void register_event(const MapEvent& ev, EventTrigger default_trigger = EventTrigger::ActionButton);

    /**
     * @brief Entfernt alle registrierten Events (Kartenwechsel, Neubeginn).
     */
    void clear() noexcept;

    /**
     * @brief Update-Schritt: Prüft Trigger-Bedingungen und startet Interpreter bei Bedarf.
     * @param dt Delta-Time (Sekunden); aktuell für Frame-Zählung und Wait-Befehle.
     */
    void update(float dt);

    /**
     * @brief Entnimmt alle von Interpreter erzeugten Requests (Choice, Battle, Shop, …).
     * Nach dem Entnehmen werden sie im Interpreter zurückgesetzt.
     * @return Liste der Pending-Requests für RuntimeState.
     */
    [[nodiscard]] std::vector<EventRequest> drain_requests();

    /**
     * @brief Prüft, ob mindestens ein Event aktiv / registriert ist.
     */
    [[nodiscard]] bool has_active_events() const noexcept;

    /**
     * @brief Anzahl registrierter Events auf der aktuellen Karte.
     */
    [[nodiscard]] usize active_count() const noexcept;

    /**
     * @brief Gibt Zugriff auf den internen Interpreter (für Runtime-State-Integration).
     */
    [[nodiscard]] EventInterpreter* interpreter() noexcept { return interpreter_; }
    [[nodiscard]] const EventInterpreter* interpreter() const noexcept { return interpreter_; }

private:
    EventInterpreter* interpreter_ = nullptr;

    struct RegisteredEvent {
        MapEvent event;
        EventTrigger trigger = EventTrigger::ActionButton;
        bool triggered_this_frame = false;
    };

    std::unordered_map<EntityId, RegisteredEvent> events_;
};

} // namespace aether::game
