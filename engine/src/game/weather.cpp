/**
 * @file weather.cpp
 */
#include <aether/game/weather.hpp>

#include <algorithm>
#include <cmath>

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

/** @brief Deterministischer Pseudo-Zufall [-1, 1]. */
f32 hash01(usize i) noexcept {
    const f64 x = std::sin(static_cast<f64>(i) * 127.1 + 311.7) * 43758.5453;
    return static_cast<f32>(x - std::floor(x)); // [0,1)
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
        sync_particle_count();
    }
}

void WeatherSystem::clear(f32 fade_seconds) {
    set(WeatherType::None, 0.0f, fade_seconds);
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

void WeatherSystem::sync_particle_count() {
    const usize want =
        current_.type == WeatherType::None
            ? 0u
            : static_cast<usize>(std::lround(current_.power * 25.0f));
    particles_.resize(want);
    // Neu angelegte Partikel (vel == 0) werden beim nächsten update() respawnt
}

void WeatherSystem::respawn_particle(WeatherParticle& p, const render::Vec3& center) {
    // Deterministische Seeds aus einem stabilen Partikel-Index ableiten:
    const usize idx = static_cast<usize>(&p - particles_.data());
    const f32 spread = 22.0f;
    const f32 top = center.y + 12.0f + hash01(idx + 7u) * 6.0f;
    p.pos = {center.x + (hash01(idx + 1u) * 2.0f - 1.0f) * spread, top,
             center.z + (hash01(idx + 2u) * 2.0f - 1.0f) * spread};
    switch (current_.type) {
    case WeatherType::Rain:
        p.vel = {0.2f * (hash01(idx + 3u) - 0.5f), -14.0f - hash01(idx + 4u) * 4.0f, 0.0f};
        break;
    case WeatherType::Storm:
        p.vel = {1.5f * (hash01(idx + 3u) - 0.5f), -20.0f - hash01(idx + 4u) * 5.0f,
                 1.5f * (hash01(idx + 5u) - 0.5f)};
        break;
    case WeatherType::Snow:
        p.vel = {0.6f * (hash01(idx + 3u) - 0.5f), -1.2f - hash01(idx + 4u), 0.0f};
        break;
    case WeatherType::Fog:
        p.vel = {1.5f * (hash01(idx + 3u) - 0.5f), 0.3f * (hash01(idx + 4u) - 0.5f),
                 1.5f * (hash01(idx + 5u) - 0.5f)};
        break;
    default:
        p.vel = {0.0f, -1.0f, 0.0f};
        break;
    }
}

void WeatherSystem::update(f64 dt, const render::Vec3& center) {
    if (fade_t_ < 1.0f) {
        fade_t_ += static_cast<f32>(dt) / std::max(fade_dur_, 1.0e-3f);
        fade_t_ = std::clamp(fade_t_, 0.0f, 1.0f);
        const f32 u = fade_t_;
        current_.type = target_.type; // Typ sofort, Stärke blendet
        current_.power = current_.power + (target_.power - current_.power) * u;
        current_.tint.r = current_.tint.r + (target_.tint.r - current_.tint.r) * u;
        current_.tint.g = current_.tint.g + (target_.tint.g - current_.tint.g) * u;
        current_.tint.b = current_.tint.b + (target_.tint.b - current_.tint.b) * u;
        if (fade_t_ >= 1.0f) {
            current_ = target_;
        }
        sync_particle_count();
    }

    if (particles_.empty()) {
        return;
    }
    acc_ += dt;
    const f32 s = static_cast<f32>(acc_);
    acc_ = 0.0;
    const f32 dtf = static_cast<f32>(dt);

    for (usize i = 0; i < particles_.size(); ++i) {
        WeatherParticle& p = particles_[i];
        if (p.vel == render::Vec3{0.0f} || p.pos.y < -0.5f) {
            respawn_particle(p, center);
        }
        // Bewegung (Snow: Sinus-Schwanken)
        p.pos += p.vel * dtf;
        if (current_.type == WeatherType::Snow) {
            p.pos.x += std::sin(s * 1.5f + static_cast<f32>(i)) * 0.02f;
        }
        if (current_.type == WeatherType::Fog) {
            p.pos.y += std::sin(s * 0.7f + static_cast<f32>(i)) * 0.01f;
        }
        // unter den Boden gefallen oder zu weit weg → neu spawnen
        if (p.pos.y < -0.5f) {
            respawn_particle(p, center);
        }
        if (current_.type == WeatherType::Fog) {
            const f32 dx = p.pos.x - center.x;
            const f32 dz = p.pos.z - center.z;
            if (dx * dx + dz * dz > 26.0f * 26.0f) {
                respawn_particle(p, center);
            }
        }
    }
}

} // namespace aether::game
