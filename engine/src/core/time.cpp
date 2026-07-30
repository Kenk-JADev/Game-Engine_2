/**
 * @file time.cpp
 * @brief Zeitmessung und ScopedTimer.
 */
#include <aether/core/time.hpp>
#include <aether/core/logger.hpp>

#include <algorithm>
#include <string>

namespace aether::core {

TimeSystem::TimeSystem(f64 fixed_delta)
    : fixed_delta_(fixed_delta > 0.0 ? fixed_delta : (1.0 / 60.0)) {
    reset();
}

void TimeSystem::reset() noexcept {
    last_ = Clock::now();
    delta_ = 0.0;
    total_ = 0.0;
    accumulator_ = 0.0;
    frame_count_ = 0;
    fixed_step_count_ = 0;
}

f64 TimeSystem::update() noexcept {
    const TimePoint now = Clock::now();
    f64 raw = seconds(now - last_);
    last_ = now;

    if (raw < 0.0) {
        raw = 0.0;
    }
    if (raw > max_delta_) {
        raw = max_delta_;
    }

    delta_ = raw * time_scale_;
    total_ += delta_;
    accumulator_ += delta_;
    ++frame_count_;
    return delta_;
}

ScopedTimer::ScopedTimer(std::string_view label)
    : label_(label)
    , start_(Clock::now()) {}

ScopedTimer::~ScopedTimer() {
    const f64 sec = elapsed_seconds();
    // Nur loggen wenn Label gesetzt; Format ohne Heap wo möglich
    std::string msg;
    msg.reserve(label_.size() + 32);
    msg.append(label_);
    msg.append(" took ");
    msg.append(std::to_string(sec * 1000.0));
    msg.append(" ms");
    log_debug("Timer", msg);
}

f64 ScopedTimer::elapsed_seconds() const noexcept {
    return seconds(Clock::now() - start_);
}

} // namespace aether::core
