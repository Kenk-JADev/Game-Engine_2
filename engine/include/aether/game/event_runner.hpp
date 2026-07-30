/**
 * @file event_runner.hpp
 * @brief Autorun / PlayerTouch / Parallel Event-Auslösung auf der Map.
 */
#pragma once

#include <aether/core/types.hpp>
#include <aether/game/event_system.hpp>
#include <aether/render/math.hpp>
#include <aether/scene/scene.hpp>

#include <unordered_set>
#include <vector>

namespace aether::game {

/**
 * @brief Scannt die Scene und startet passende Event-Seiten.
 *
 * - Autorun: einmal pro Map-Load (bis Seite fertig)
 * - PlayerTouch: wenn Spieler-AABB Event-Trigger berührt
 * - ActionButton: weiterhin über PlayerController / Runtime Confirm
 * - Parallel: startet wenn noch nicht laufend (nicht-blockierend parallel
 *   wird vereinfacht als Autorun behandelt)
 */
class MapEventRunner {
public:
    void reset_map();

    /**
     * @brief Pro Fixed-Update aufrufen.
     * @param player_pos aktuelle Spielerposition
     * @param player_radius grobe Touch-Distanz
     * @param interpreter darf start() nur wenn !is_running()
     * @return true wenn ein Event gestartet wurde
     */
    bool update(scene::Scene& scene, const render::Vec3& player_pos, f32 player_radius,
                EventInterpreter& interpreter, GameState& state);

    [[nodiscard]] const std::unordered_set<EntityId>& fired_autorun() const {
        return autorun_done_;
    }

private:
    [[nodiscard]] static bool page_conditions_met(const EventPage& page,
                                                  const GameState& state);
    [[nodiscard]] static const EventPage* select_page(const MapEvent& ev,
                                                      const GameState& state);

    std::unordered_set<EntityId> autorun_done_;
    std::unordered_set<EntityId> touch_cooldown_; ///< currently overlapping
};

/**
 * @brief Einfacher Map-Übergangs-Fade (0..1).
 */
class ScreenFade {
public:
    void fade_out(f32 seconds);
    void fade_in(f32 seconds);
    void update(f64 dt);
    [[nodiscard]] f32 alpha() const noexcept { return alpha_; }
    [[nodiscard]] bool busy() const noexcept { return mode_ != Mode::Idle; }
    [[nodiscard]] bool just_black() const noexcept {
        return mode_ == Mode::Out && alpha_ >= 0.999f;
    }

private:
    enum class Mode { Idle, Out, In } mode_ = Mode::Idle;
    f32 alpha_ = 0.0f;
    f32 speed_ = 1.0f;
};

} // namespace aether::game
