/**
 * @file event_runner.cpp
 */
#include <aether/game/event_runner.hpp>
#include <aether/render/math.hpp>

#include <algorithm>
#include <cmath>

namespace aether::game {

void MapEventRunner::reset_map() {
    autorun_done_.clear();
    touch_cooldown_.clear();
}

bool MapEventRunner::page_conditions_met(const EventPage& page, const GameState& state) {
    if (!page.conditions.is_object()) {
        return true;
    }
    if (page.conditions.contains("switch_id")) {
        const u32 id = page.conditions["switch_id"].get<u32>();
        const bool need = page.conditions.value("switch_value", true);
        if (state.get_switch(id) != need) {
            return false;
        }
    }
    if (page.conditions.contains("variable_id")) {
        const u32 id = page.conditions["variable_id"].get<u32>();
        const i32 need = page.conditions.value("variable_value", 0);
        const std::string op = page.conditions.value("op", ">=");
        const i32 v = state.get_variable(id);
        if (op == ">=" && !(v >= need)) return false;
        if (op == "==" && !(v == need)) return false;
        if (op == ">" && !(v > need)) return false;
        if (op == "<" && !(v < need)) return false;
    }
    return true;
}

const EventPage* MapEventRunner::select_page(const MapEvent& ev, const GameState& state) {
    // last matching page wins (RPG Maker style)
    const EventPage* best = nullptr;
    for (const auto& p : ev.pages) {
        if (page_conditions_met(p, state)) {
            best = &p;
        }
    }
    return best;
}

bool MapEventRunner::update(scene::Scene& scene, const render::Vec3& player_pos,
                            f32 player_radius, EventInterpreter& interpreter,
                            GameState& state) {
    if (interpreter.is_running()) {
        return false;
    }

    bool started = false;
    std::unordered_set<EntityId> now_touch;

    for (const auto& o : scene.objects()) {
        if (!o.map_event || o.map_event->pages.empty()) {
            continue;
        }
        const EventPage* page = select_page(*o.map_event, state);
        if (!page || page->commands.empty()) {
            continue;
        }

        const render::Vec3 ep{o.transform.position.x, 0.0f, o.transform.position.z};
        const f32 dist =
            glm::length(render::Vec3{player_pos.x - ep.x, 0.0f, player_pos.z - ep.z});
        const bool near = dist <= player_radius + 0.6f;

        if (page->trigger == EventTrigger::Autorun ||
            page->trigger == EventTrigger::Parallel) {
            if (autorun_done_.count(o.id) == 0) {
                interpreter.start(page->commands);
                autorun_done_.insert(o.id);
                started = true;
                break;
            }
        } else if (page->trigger == EventTrigger::PlayerTouch ||
                   page->trigger == EventTrigger::EventTouch) {
            if (near) {
                now_touch.insert(o.id);
                if (touch_cooldown_.count(o.id) == 0) {
                    interpreter.start(page->commands);
                    touch_cooldown_.insert(o.id);
                    started = true;
                    break;
                }
            }
        }
    }

    // clear cooldown when left
    for (auto it = touch_cooldown_.begin(); it != touch_cooldown_.end();) {
        if (now_touch.count(*it) == 0) {
            it = touch_cooldown_.erase(it);
        } else {
            ++it;
        }
    }
    return started;
}

void ScreenFade::fade_out(f32 seconds) {
    mode_ = Mode::Out;
    speed_ = seconds > 1.0e-3f ? (1.0f / seconds) : 1000.0f;
}

void ScreenFade::fade_in(f32 seconds) {
    mode_ = Mode::In;
    alpha_ = 1.0f;
    speed_ = seconds > 1.0e-3f ? (1.0f / seconds) : 1000.0f;
}

void ScreenFade::update(f64 dt) {
    if (mode_ == Mode::Idle) {
        return;
    }
    const f32 d = static_cast<f32>(dt) * speed_;
    if (mode_ == Mode::Out) {
        alpha_ = std::min(1.0f, alpha_ + d);
        if (alpha_ >= 1.0f) {
            mode_ = Mode::Idle;
        }
    } else if (mode_ == Mode::In) {
        alpha_ = std::max(0.0f, alpha_ - d);
        if (alpha_ <= 0.0f) {
            mode_ = Mode::Idle;
        }
    }
}

} // namespace aether::game
