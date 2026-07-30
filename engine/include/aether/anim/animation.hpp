/**
 * @file animation.hpp
 * @brief Einfache Tweens und Transform-Animationen (stilisierte RPGs).
 */
#pragma once

#include <aether/core/types.hpp>
#include <aether/render/math.hpp>

#include <functional>
#include <string>
#include <vector>

namespace aether::anim {

enum class Ease {
    Linear,
    QuadIn,
    QuadOut,
    QuadInOut,
    SmoothStep,
};

[[nodiscard]] f32 ease_apply(Ease e, f32 t) noexcept;

/**
 * @brief Generischer Float-Tween.
 */
struct Tween {
    f32 from = 0.0f;
    f32 to = 1.0f;
    f32 duration = 1.0f;
    f32 elapsed = 0.0f;
    Ease ease = Ease::Linear;
    bool loop = false;
    bool finished = false;

    [[nodiscard]] f32 value() const noexcept;
    void update(f32 dt) noexcept;
    void reset() noexcept;
};

/**
 * @brief Keyframe-Animation für Position/Scale/Rotation-Y.
 */
struct TransformTrack {
    struct Key {
        f32 time = 0.0f;
        render::Vec3 position{0.0f};
        render::Vec3 scale{1.0f};
        f32 yaw_degrees = 0.0f;
    };
    std::vector<Key> keys;
    f32 duration = 0.0f;
    bool loop = true;

    [[nodiscard]] render::Transform sample(f32 time) const;
};

/**
 * @brief Laufende Animation auf einem Transform.
 */
class Animator {
public:
    void play(TransformTrack track, bool restart = true);
    void stop();
    void update(f32 dt);
    void apply(render::Transform& out) const;

    [[nodiscard]] bool playing() const noexcept { return playing_; }
    [[nodiscard]] f32 time() const noexcept { return time_; }

private:
    TransformTrack track_{};
    f32 time_ = 0.0f;
    bool playing_ = false;
};

/**
 * @brief Einfache Bounce/Idle-Hilfsanimation.
 */
[[nodiscard]] TransformTrack make_idle_bob(f32 amplitude = 0.15f, f32 period = 2.0f);

/**
 * @brief Tween-Gruppe für UI/Wetter-Fades.
 */
class TweenPlayer {
public:
    usize add(Tween t);
    void update(f32 dt);
    [[nodiscard]] f32 value(usize id) const;
    [[nodiscard]] bool finished(usize id) const;
    void clear();

private:
    std::vector<Tween> tweens_;
};

} // namespace aether::anim
