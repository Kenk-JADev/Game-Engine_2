/**
 * @file event_bus.hpp
 * @brief Typsicherer, synchroner Event-Bus für Engine-interne Signale.
 */
#pragma once

#include <aether/core/config.hpp>
#include <aether/core/types.hpp>

#include <functional>
#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace aether::core {

using ListenerId = u64;

/**
 * @brief Minimaler Pub/Sub ohne RTTI-pflichtige Nutzdaten-Kopien jenseits von T.
 *
 * Listener werden synchron im publish()-Aufrufer-Thread ausgeführt.
 * Für Editor/Engine-Entkopplung gedacht – nicht für High-Frequency-Gameplay.
 */
class EventBus : public NonMovable {
public:
    EventBus() = default;
    ~EventBus() = default;

    /**
     * @brief Registriert einen Listener für Event-Typ E.
     * @return ID zum späteren unsubscribe
     */
    template <typename E, typename Fn>
    ListenerId subscribe(Fn&& fn) {
        std::lock_guard lock(mutex_);
        const ListenerId id = next_id_++;
        auto& vec = listeners_[std::type_index(typeid(E))];
        vec.push_back(Entry{
            id,
            [f = std::forward<Fn>(fn)](const void* e) {
                f(*static_cast<const E*>(e));
            },
        });
        return id;
    }

    /** @brief Entfernt einen Listener. */
    void unsubscribe(ListenerId id);

    /** @brief Publiziert ein Event an alle Listener von E. */
    template <typename E>
    void publish(const E& event) {
        std::vector<Handler> snapshot;
        {
            std::lock_guard lock(mutex_);
            const auto it = listeners_.find(std::type_index(typeid(E)));
            if (it == listeners_.end()) {
                return;
            }
            snapshot.reserve(it->second.size());
            for (const auto& e : it->second) {
                snapshot.push_back(e.handler);
            }
        }
        for (auto& h : snapshot) {
            h(&event);
        }
    }

    /** @brief Entfernt alle Listener. */
    void clear();

private:
    using Handler = std::function<void(const void*)>;
    struct Entry {
        ListenerId id;
        Handler handler;
    };

    std::unordered_map<std::type_index, std::vector<Entry>> listeners_;
    ListenerId next_id_ = 1;
    mutable std::mutex mutex_;
};

// -----------------------------------------------------------------------------
// Einige Standard-Engine-Events (erweiterbar)
// -----------------------------------------------------------------------------

struct EngineBootEvent {
    AppMode mode{};
};

struct EngineShutdownEvent {};

struct FrameBeginEvent {
    f64 delta_seconds = 0.0;
    u64 frame_index = 0;
};

struct FrameEndEvent {
    f64 delta_seconds = 0.0;
    u64 frame_index = 0;
};

} // namespace aether::core
