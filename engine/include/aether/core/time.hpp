/**
 * @file time.hpp
 * @brief Zeitmessung, Delta-Time und fester Simulations-Tick.
 */
#pragma once

#include <aether/core/types.hpp>

#include <chrono>

namespace aether::core {

using Clock     = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using Duration  = Clock::duration;

/**
 * @brief Wandelt Duration in Sekunden (f64) um.
 */
[[nodiscard]] inline f64 seconds(Duration d) noexcept {
    return std::chrono::duration<f64>(d).count();
}

/**
 * @brief Wandelt Duration in Millisekunden (f64) um.
 */
[[nodiscard]] inline f64 milliseconds(Duration d) noexcept {
    return std::chrono::duration<f64, std::milli>(d).count();
}

/**
 * @brief Frame-/Spielzeit-Dienst.
 *
 * update() einmal pro Frame auf dem Main-Thread aufrufen.
 */
class TimeSystem {
public:
    explicit TimeSystem(f64 fixed_delta = 1.0 / 60.0);

    /** @brief Startet bzw. setzt die Uhr zurück. */
    void reset() noexcept;

    /**
     * @brief Aktualisiert Delta- und Gesamtzeit.
     * @return verstrichene Realzeit seit letztem update (Sekunden, geclampt)
     */
    f64 update() noexcept;

    /**
     * @brief Konsumiert akkumulierte Fixed-Ticks.
     * @param fn Callback pro Tick mit fixed_delta
     */
    template <typename Fn>
    void drain_fixed_steps(Fn&& fn) {
        constexpr int kMaxSteps = 8; // spiral-of-death Schutz
        int steps = 0;
        while (accumulator_ >= fixed_delta_ && steps < kMaxSteps) {
            fn(fixed_delta_);
            accumulator_ -= fixed_delta_;
            ++fixed_step_count_;
            ++steps;
        }
        // Überschüssige Zeit verwerfen, falls zu groß
        if (steps == kMaxSteps) {
            accumulator_ = 0.0;
        }
    }

    [[nodiscard]] f64 delta_seconds() const noexcept { return delta_; }
    [[nodiscard]] f64 total_seconds() const noexcept { return total_; }
    [[nodiscard]] f64 fixed_delta() const noexcept { return fixed_delta_; }
    [[nodiscard]] f64 alpha() const noexcept {
        // Interpolationsfaktor für Rendering zwischen Fixed-Steps
        return fixed_delta_ > 0.0 ? (accumulator_ / fixed_delta_) : 0.0;
    }
    [[nodiscard]] u64 frame_count() const noexcept { return frame_count_; }
    [[nodiscard]] u64 fixed_step_count() const noexcept { return fixed_step_count_; }

    /** @brief Maximal erlaubtes Frame-Delta (Spikes glätten). Default 0.1s */
    void set_max_delta(f64 seconds) noexcept { max_delta_ = seconds; }

    void set_time_scale(f64 scale) noexcept { time_scale_ = scale < 0.0 ? 0.0 : scale; }
    [[nodiscard]] f64 time_scale() const noexcept { return time_scale_; }

private:
    TimePoint last_{};
    f64 delta_ = 0.0;
    f64 total_ = 0.0;
    f64 fixed_delta_ = 1.0 / 60.0;
    f64 accumulator_ = 0.0;
    f64 max_delta_ = 0.1;
    f64 time_scale_ = 1.0;
    u64 frame_count_ = 0;
    u64 fixed_step_count_ = 0;
};

/**
 * @brief Einfache Scope-Stoppuhr für Profiling (Debug).
 */
class ScopedTimer {
public:
    explicit ScopedTimer(std::string_view label);
    ~ScopedTimer();

    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;

    [[nodiscard]] f64 elapsed_seconds() const noexcept;

private:
    std::string_view label_;
    TimePoint start_;
};

} // namespace aether::core
