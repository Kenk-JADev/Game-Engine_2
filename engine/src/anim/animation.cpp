/**
 * @file animation.cpp
 */
#include <aether/anim/animation.hpp>

#include <algorithm>
#include <cmath>

namespace aether::anim {

f32 ease_apply(Ease e, f32 t) noexcept {
    t = std::clamp(t, 0.0f, 1.0f);
    switch (e) {
    case Ease::Linear:
        return t;
    case Ease::QuadIn:
        return t * t;
    case Ease::QuadOut:
        return t * (2.0f - t);
    case Ease::QuadInOut:
        return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
    case Ease::SmoothStep:
        return t * t * (3.0f - 2.0f * t);
    }
    return t;
}

f32 Tween::value() const noexcept {
    if (duration <= 1.0e-6f) {
        return to;
    }
    const f32 t = ease_apply(ease, elapsed / duration);
    return from + (to - from) * t;
}

void Tween::update(f32 dt) noexcept {
    if (finished) {
        return;
    }
    elapsed += dt;
    if (elapsed >= duration) {
        if (loop) {
            elapsed = std::fmod(elapsed, duration);
        } else {
            elapsed = duration;
            finished = true;
        }
    }
}

void Tween::reset() noexcept {
    elapsed = 0.0f;
    finished = false;
}

render::Transform TransformTrack::sample(f32 time) const {
    render::Transform out;
    if (keys.empty()) {
        return out;
    }
    if (keys.size() == 1) {
        out.position = keys[0].position;
        out.scale = keys[0].scale;
        out.rotation = glm::angleAxis(render::radians(keys[0].yaw_degrees),
                                      render::Vec3{0, 1, 0});
        return out;
    }
    f32 t = time;
    if (duration > 0.0f) {
        if (loop) {
            t = std::fmod(t, duration);
            if (t < 0.0f) t += duration;
        } else {
            t = std::clamp(t, 0.0f, duration);
        }
    }
    // find segment
    usize i = 0;
    while (i + 1 < keys.size() && keys[i + 1].time < t) {
        ++i;
    }
    const Key& a = keys[i];
    const Key& b = keys[std::min(i + 1, keys.size() - 1)];
    f32 u = 0.0f;
    if (b.time > a.time) {
        u = (t - a.time) / (b.time - a.time);
    }
    u = ease_apply(Ease::SmoothStep, u);
    out.position = glm::mix(a.position, b.position, u);
    out.scale = glm::mix(a.scale, b.scale, u);
    const f32 yaw = a.yaw_degrees + (b.yaw_degrees - a.yaw_degrees) * u;
    out.rotation = glm::angleAxis(render::radians(yaw), render::Vec3{0, 1, 0});
    return out;
}

void Animator::play(TransformTrack track, bool restart) {
    track_ = std::move(track);
    if (restart) {
        time_ = 0.0f;
    }
    playing_ = !track_.keys.empty();
}

void Animator::stop() {
    playing_ = false;
}

void Animator::update(f32 dt) {
    if (!playing_) {
        return;
    }
    time_ += dt;
    if (!track_.loop && track_.duration > 0.0f && time_ >= track_.duration) {
        time_ = track_.duration;
        playing_ = false;
    }
}

void Animator::apply(render::Transform& out) const {
    if (track_.keys.empty()) {
        return;
    }
    out = track_.sample(time_);
}

TransformTrack make_idle_bob(f32 amplitude, f32 period) {
    TransformTrack tr;
    tr.loop = true;
    tr.duration = period;
    tr.keys = {
        {0.0f, {0, 0, 0}, {1, 1, 1}, 0},
        {period * 0.5f, {0, amplitude, 0}, {1, 1, 1}, 0},
        {period, {0, 0, 0}, {1, 1, 1}, 0},
    };
    return tr;
}

usize TweenPlayer::add(Tween t) {
    tweens_.push_back(t);
    return tweens_.size() - 1;
}

void TweenPlayer::update(f32 dt) {
    for (auto& tw : tweens_) {
        tw.update(dt);
    }
}

f32 TweenPlayer::value(usize id) const {
    if (id >= tweens_.size()) {
        return 0.0f;
    }
    return tweens_[id].value();
}

bool TweenPlayer::finished(usize id) const {
    if (id >= tweens_.size()) {
        return true;
    }
    return tweens_[id].finished;
}

void TweenPlayer::clear() {
    tweens_.clear();
}

} // namespace aether::anim
