/**
 * @file weather.hpp
 * @brief Einfaches Wetter (visueller Tint + Stärke) – kein Partikel-Zwang.
 */
#pragma once

#include <aether/core/types.hpp>
#include <aether/render/color.hpp>

#include <string>

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

class WeatherSystem {
public:
    void set(WeatherType type, f32 power = 5.0f, f32 fade_seconds = 0.0f);
    void clear(f32 fade_seconds = 0.0f);
    void update(f64 dt);

    [[nodiscard]] const WeatherState& state() const noexcept { return current_; }
    [[nodiscard]] render::Color clear_color_mod(const render::Color& base) const;

    static WeatherType from_string(std::string_view s) noexcept;
    static const char* to_string(WeatherType t) noexcept;

private:
    WeatherState current_{};
    WeatherState target_{};
    f32 fade_t_ = 1.0f;
    f32 fade_dur_ = 0.0f;
};

} // namespace aether::game
