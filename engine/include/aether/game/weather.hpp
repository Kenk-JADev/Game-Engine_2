/**
 * @file weather.hpp
 * @brief Einfaches Wetter (visueller Tint + Stärke) – kein Partikel-Zwang.
 */
#pragma once

#include <aether/core/types.hpp>
#include <aether/render/color.hpp>
#include <aether/render/math.hpp>

#include <string>
#include <vector>

namespace aether::game {

enum class WeatherType {
    None,
    Rain,
    Storm,
    Snow,
    Fog,
};

struct WeatherState {
    WeatherType type = WeatherType::None;
    f32 power = 0.0f; ///< 0–10
    render::Color tint = render::Color::white();
};

/** @brief Ein Niederschlags-/Nebelpartikel (Weltkoordinaten). */
struct WeatherParticle {
    render::Vec3 pos{0.0f};
    render::Vec3 vel{0.0f};
};

class WeatherSystem {
public:
    void set(WeatherType type, f32 power = 5.0f, f32 fade_seconds = 0.0f);
    void clear(f32 fade_seconds = 0.0f);

    /**
     * @brief Tick: Fade + Partikelbewegung/-spawning um `center` (meist Spieler).
     */
    void update(f64 dt, const render::Vec3& center = {0.0f, 0.0f, 0.0f});

    [[nodiscard]] const WeatherState& state() const noexcept { return current_; }
    [[nodiscard]] render::Color clear_color_mod(const render::Color& base) const;

    /**
     * @brief Aktive Partikelpositionen (Runtime rendert sie als kleine Quads).
     */
    [[nodiscard]] const std::vector<WeatherParticle>& particles() const noexcept {
        return particles_;
    }
    [[nodiscard]] usize particle_count() const noexcept { return particles_.size(); }

    static WeatherType from_string(std::string_view s) noexcept;
    static const char* to_string(WeatherType t) noexcept;

private:
    void respawn_particle(WeatherParticle& p, const render::Vec3& center);
    void sync_particle_count();

    WeatherState current_{};
    WeatherState target_{};
    f32 fade_t_ = 1.0f;
    f32 fade_dur_ = 0.0f;
    std::vector<WeatherParticle> particles_;
    f64 acc_ = 0.0;
};

} // namespace aether::game
