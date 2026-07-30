/**
 * @file weather.cpp
 */
#include <aether/game/weather.hpp>

#include <algorithm>

namespace aether::game {
namespace {

render::Color tint_for(WeatherType t, f32 power) {
    const f32 p = std::clamp(power / 10.0f, 0.0f, 1.0f);
    switch (t) {
    case WeatherType::Rain:
        return render::Color{0.65f, 0.70f, 0.80f, 1.0f};
    case WeatherType::Storm:
        return render::Color{0.45f, 0.48f, 0.55f, 1.0f};
    case WeatherType::Snow:
        return render::Color{0.85f, 0.90f, 0.95f, 1.0f};
    case WeatherType::Fog:
        return render::Color{0.75f, 0.78f, 0.80f, 1.0f};
    case WeatherType::None:
    default:
        return render::Color::white();
    }
    (void)p;
}

} // namespace

WeatherType WeatherSystem::from_string(std::string_view s) noexcept {
    if (s == "rain") return WeatherType::Rain;
    if (s == "storm") return WeatherType::Storm;
    if (s == "snow") return WeatherType::Snow;
    if (s == "fog") return WeatherType::Fog;
    return WeatherType::None;
}

const char* WeatherSystem::to_string(WeatherType t) noexcept {
    switch (t) {
    case WeatherType::Rain: return "rain";
    case WeatherType::Storm: return "storm";
    case WeatherType::Snow: return "snow";
    case WeatherType::Fog: return "fog";
    case WeatherType::None:
    default: return "none";
    }
}

void WeatherSystem::set(WeatherType type, f32 power, f32 fade_seconds) {
    target_.type = type;
    target_.power = std::clamp(power, 0.0f, 10.0f);
    target_.tint = tint_for(type, target_.power);
    fade_dur_ = std::max(0.0f, fade_seconds);
    fade_t_ = fade_dur_ <= 0.0f ? 1.0f : 0.0f;
    if (fade_dur_ <= 0.0f) {
        current_ = target_;
    }
}

void WeatherSystem::clear(f32 fade_seconds) {
    set(WeatherType::None, 0.0f, fade_seconds);
}

void WeatherSystem::update(f64 dt) {
    if (fade_t_ >= 1.0f) {
        current_ = target_;
        return;
    }
    fade_t_ += static_cast<f32>(dt) / std::max(fade_dur_, 1.0e-3f);
    fade_t_ = std::clamp(fade_t_, 0.0f, 1.0f);
    const f32 u = fade_t_;
    current_.power = current_.power + (target_.power - current_.power) * u;
    current_.tint.r = current_.tint.r + (target_.tint.r - current_.tint.r) * u;
    current_.tint.g = current_.tint.g + (target_.tint.g - current_.tint.g) * u;
    current_.tint.b = current_.tint.b + (target_.tint.b - current_.tint.b) * u;
    if (fade_t_ >= 1.0f) {
        current_.type = target_.type;
        current_.tint = target_.tint;
        current_.power = target_.power;
    }
}

render::Color WeatherSystem::clear_color_mod(const render::Color& base) const {
    const f32 p = std::clamp(current_.power / 10.0f, 0.0f, 1.0f) * 0.35f;
    return render::Color{
        base.r * (1.0f - p) + current_.tint.r * p,
        base.g * (1.0f - p) + current_.tint.g * p,
        base.b * (1.0f - p) + current_.tint.b * p,
        base.a,
    };
}

} // namespace aether::game
