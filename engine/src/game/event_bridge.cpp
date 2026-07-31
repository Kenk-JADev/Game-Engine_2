/**
 * @file event_bridge.cpp
 * @brief Implementierung der EventBridge (ADR-001).
 *
 * @author Senior Software Architect / Lead Engine Developer
 */
#include <aether/game/event_bridge.hpp>
#include <aether/core/logger.hpp>

namespace aether::game {

EventBridge::EventBridge(EventInterpreter* interpreter) noexcept
    : interpreter_(interpreter) {
    if (!interpreter_) {
        core::log_warn("EventBridge", "Interpreter ist nullptr – Bridge nicht funktional.");
    }
}

void EventBridge::register_event(const MapEvent& ev, EventTrigger default_trigger) {
    if (ev.id == kInvalidEntity) {
        core::log_warn("EventBridge", "Event ohne gültige ID ignoriert (name=" + ev.name + ").");
        return;
    }
    RegisteredEvent reg;
    reg.event = ev;
    reg.trigger = default_trigger;
    reg.triggered_this_frame = false;

    // Wenn Event bereits existiert, überschreiben (Editor-Update-Szenario).
    events_[ev.id] = std::move(reg);
    core::log_info("EventBridge", "Registriert Event id=" + std::to_string(ev.id) +
                    " name=\"" + ev.name + "\" pages=" + std::to_string(ev.pages.size()));
}

void EventBridge::clear() noexcept {
    events_.clear();
    core::log_info("EventBridge", "Alle Events gelöscht (Kartenwechsel / Clear).");
}

void EventBridge::update(float dt) {
    (void)dt; // Für zukünftige Frame-basierte Wait-Befehle und Parallel-Event-Timing reserviert.

    if (!interpreter_) {
        return;
    }

    // Wenn Interpreter bereits läuft, lassen wir ihn weiterlaufen (Parallel-Events / Wait).
    // Neue Events werden nur gestartet, wenn Interpreter nicht aktiv ist.
    bool interpreter_busy = interpreter_->is_running();

    for (auto& [id, reg] : events_) {
        // Reset des Frame-Flags
        reg.triggered_this_frame = false;

        // Trigger-Bedingung prüfen.
        // Simplifiziert für Milestone 01: ActionButton wird als "bereit" betrachtet,
        // wenn Interpreter nicht läuft und Event mindestens eine Seite hat.
        // In vollständiger Implementierung: Abfrage von InputManager / Touch-State / Bedingungen.
        bool trigger_fired = false;

        if (!interpreter_busy) {
            // Prüfe, ob das Event mindestens eine Seite mit Befehlen hat.
            if (!reg.event.pages.empty()) {
                // Für Milestone 01: Jede registrierte Seite mit Kommandos ist "triggerbereit".
                // Der Editor entscheidet über Trigger-Typ (ActionButton / Autorun / Parallel).
                if (reg.trigger == EventTrigger::Autorun) {
                    trigger_fired = true; // Autorun startet sofort, wenn Interpreter frei.
                } else if (reg.trigger == EventTrigger::Parallel) {
                    // Parallel-Events laufen separat; hier als Vorbereitung registriert.
                    trigger_fired = false; // Separate Parallel-Logik wäre nötig.
                } else if (reg.trigger == EventTrigger::ActionButton ||
                           reg.trigger == EventTrigger::PlayerTouch ||
                           reg.trigger == EventTrigger::EventTouch) {
                    // Für Milestone 01: Wir simulieren, dass der Trigger erfüllt ist,
                    // wenn eine Interaktion stattfindet. Die echte Eingabe kommt vom InputManager.
                    // Hier: Wir starten das Toplevel-Event, wenn es nicht schon läuft.
                    trigger_fired = true; // Vereinfachte Annahme für Integrationstest / Smoke.
                }
            }
        }

        if (trigger_fired) {
            reg.triggered_this_frame = true;
            // Wähle die aktive Seite (Seite 0 als Default; in vollständiger Implementierung:
            // Bedingungen prüfen und richtige Seite wählen).
            int page_index = 0;
            if (page_index >= 0 && static_cast<usize>(page_index) < reg.event.pages.size()) {
                const auto& page = reg.event.pages[page_index];
                if (!page.commands.empty()) {
                    interpreter_->start(page.commands);
                    interpreter_busy = true; // Interpreter jetzt aktiv.
                    core::log_info("EventBridge", "Starte Interpreter für Event id=" + std::to_string(id) +
                                    " page=" + std::to_string(page_index));
                }
            }
        }
    }

    // Interpreter aktualisieren (Führt Befehle aus, erzeugt Requests, wartet auf Wait/Choice).
    if (interpreter_ && interpreter_->is_running()) {
        interpreter_->update();
    }
}

std::vector<EventRequest> EventBridge::drain_requests() {
    std::vector<EventRequest> result;
    if (!interpreter_) {
        return result;
    }

    // Der Interpreter stellt einen Request bereit, wenn ein Befehl (Choice, Battle, Shop …)
    // eine UI-Interaktion erfordert.
    if (interpreter_->pending_request().kind != EventRequest::Kind::None) {
        result.push_back(interpreter_->pending_request());
        interpreter_->clear_pending();
        core::log_info("EventBridge", "Drain request kind=" + std::to_string(static_cast<int>(result.back().kind)));
    }
    return result;
}

bool EventBridge::has_active_events() const noexcept {
    return !events_.empty();
}

usize EventBridge::active_count() const noexcept {
    return events_.size();
}

} // namespace aether::game
