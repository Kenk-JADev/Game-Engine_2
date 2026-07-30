/**
 * @file engine_context.hpp
 * @brief Zentraler Besitz und Zugriff auf Engine-Dienste.
 *
 * Keine globalen veränderlichen Variablen für Subsysteme:
 * Dienste leben in EngineContext und werden explizit durchgereicht
 * bzw. über die aktive Context-Instanz erreicht.
 */
#pragma once

#include <aether/core/config.hpp>
#include <aether/core/event_bus.hpp>
#include <aether/core/logger.hpp>
#include <aether/core/thread_pool.hpp>
#include <aether/core/time.hpp>
#include <aether/core/types.hpp>

#include <memory>

namespace aether::core {

/**
 * @brief Lebenszyklus-Status des Context.
 */
enum class ContextState {
    Created,
    Running,
    Stopping,
    Stopped,
};

/**
 * @brief Wurzelobjekt der Engine.
 *
 * create() baut Logger, Time, ThreadPool, EventBus.
 * Weitere Subsysteme (Window, Renderer, …) werden in späteren Modulen
 * als optionale unique_ptr-Members ergänzt.
 */
class EngineContext : public NonMovable {
public:
    /**
     * @brief Erzeugt und initialisiert einen Context.
     * @throws std::runtime_error bei fatalen Startfehlern
     */
    [[nodiscard]] static std::unique_ptr<EngineContext> create(EngineConfig config);

    ~EngineContext();

    // Nicht kopier-/verschiebbar (NonMovable); unique_ptr hält Besitz.

    [[nodiscard]] ContextState state() const noexcept { return state_; }
    [[nodiscard]] const EngineConfig& config() const noexcept { return config_; }
    [[nodiscard]] EngineConfig& config() noexcept { return config_; }

    [[nodiscard]] Logger& logger() noexcept { return *logger_; }
    [[nodiscard]] const Logger& logger() const noexcept { return *logger_; }

    [[nodiscard]] TimeSystem& time() noexcept { return *time_; }
    [[nodiscard]] const TimeSystem& time() const noexcept { return *time_; }

    [[nodiscard]] ThreadPool& thread_pool() noexcept { return *thread_pool_; }
    [[nodiscard]] EventBus& events() noexcept { return *events_; }

    /**
     * @brief Markiert den Context als laufend und feuert EngineBootEvent.
     */
    void start();

    /**
     * @brief Beendet Dienste geordnet und feuert EngineShutdownEvent.
     */
    void shutdown();

    /**
     * @brief Ein Frame-Tick der Core-Dienste (Zeit + Frame-Events).
     * @return false wenn State != Running
     */
    bool pump_frame();

    // -------------------------------------------------------------------------
    // Aktiver Context (thread-local / main-thread), gesetzt von create/start
    // -------------------------------------------------------------------------
    static void set_active(EngineContext* ctx) noexcept;
    [[nodiscard]] static EngineContext* active() noexcept;

private:
    explicit EngineContext(EngineConfig config);
    void setup_logging();

    EngineConfig config_;
    ContextState state_ = ContextState::Created;

    std::unique_ptr<Logger> logger_;
    std::unique_ptr<TimeSystem> time_;
    std::unique_ptr<ThreadPool> thread_pool_;
    std::unique_ptr<EventBus> events_;
};

} // namespace aether::core
