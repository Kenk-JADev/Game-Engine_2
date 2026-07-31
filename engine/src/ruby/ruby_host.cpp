/**
 * @file ruby_host.cpp
 * @brief Ruby-Host: echte Engine-API-Bindings (Graphics, Audio, Input, …).
 *
 * Alle Host-Funktionen arbeiten über die String-Schnittstelle von
 * RubyVM::define_function, damit sowohl die Stub-VM (Headless/Tests) als
 * auch die mruby-VM denselben Code verwenden.
 */
#include <aether/ruby/ruby_host.hpp>

#include <aether/audio/audio_engine.hpp>
#include <aether/core/logger.hpp>
#include <aether/game/database.hpp>
#include <aether/game/event_system.hpp>
#include <aether/game/inventory.hpp>
#include <aether/game/player.hpp>
#include <aether/game/quest.hpp>
#include <aether/game/scene_stack.hpp>
#include <aether/game/weather.hpp>
#include <aether/input/input_manager.hpp>
#include <aether/render/camera.hpp>
#include <aether/scene/scene.hpp>

#include <algorithm>
#include <cctype>
#include <string>

namespace aether::ruby {
namespace {

using Fn = std::function<std::string(const std::vector<std::string>&)>;

} // namespace

// =============================================================================
// Hilfen
// =============================================================================

std::string RubyHost::lit(i64 v) {
    return std::to_string(v);
}

std::string RubyHost::lit(f64 v) {
    // Ganzzahlige Werte ohne Nachkommastellen ausgeben (Ruby-übliche Lesart)
    if (v == static_cast<i64>(v)) {
        return std::to_string(static_cast<i64>(v));
    }
    std::string s = std::to_string(v);
    while (!s.empty() && s.back() == '0') s.pop_back();
    if (!s.empty() && s.back() == '.') s.pop_back();
    return s;
}

std::string RubyHost::lit(bool v) {
    return v ? "true" : "false";
}

std::string RubyHost::lit_str(std::string_view v) {
    std::string out = "\"";
    for (const char c : v) {
        if (c == '"' || c == '\\') {
            out.push_back('\\');
        }
        out.push_back(c);
    }
    out.push_back('"');
    return out;
}

i64 RubyHost::arg_i64(const std::vector<std::string>& args, usize i, i64 dflt) {
    if (i >= args.size()) return dflt;
    try {
        return std::stoll(args[i]);
    } catch (...) {
        return dflt;
    }
}

f64 RubyHost::arg_f64(const std::vector<std::string>& args, usize i, f64 dflt) {
    if (i >= args.size()) return dflt;
    try {
        return std::stod(args[i]);
    } catch (...) {
        return dflt;
    }
}

u32 RubyHost::resolve_item_id(const std::string& key) const {
    // Numerisch → Item-Id
    try {
        const u64 v = std::stoull(key);
        return static_cast<u32>(v);
    } catch (...) {
    }
    // Sonst Name aus der Datenbank
    if (b_.database) {
        for (const auto& it : b_.database->items) {
            if (it.name == key) return it.id;
        }
    }
    return 0;
}

// =============================================================================
// Anfragen (Runtime konsumiert sie pro Frame)
// =============================================================================

bool RubyHost::take_transfer(u32& map_id, render::Vec3& pos, i32& dir) {
    if (!transfer_pending_) return false;
    map_id = transfer_map_;
    pos = transfer_pos_;
    dir = transfer_dir_;
    transfer_pending_ = false;
    return true;
}

bool RubyHost::take_map_load(u32& map_id) {
    if (!map_load_pending_) return false;
    map_id = map_load_id_;
    map_load_pending_ = false;
    return true;
}

std::string RubyHost::take_scene_request() {
    std::string r = scene_request_;
    scene_request_.clear();
    return r;
}

int RubyHost::take_fade_request() {
    const int r = fade_request_;
    fade_request_ = 0;
    return r;
}

// =============================================================================
// install() – Registrierung aller Module
// =============================================================================

void RubyHost::install(RubyVM& vm) {
    const auto reg = [&](std::string module, std::string name, i32 arity, Fn fn) {
        vm.define_function(HostFunction{std::move(module), std::move(name), arity,
                                        std::move(fn)});
    };

    // --- Graphics ------------------------------------------------------------
    reg("Graphics", "frame_rate=", 1, [this](const std::vector<std::string>& a) {
        b_.frame_rate = static_cast<i32>(arg_i64(a, 0, 60));
        return lit(true);
    });
    reg("Graphics", "frame_rate", 0, [this](const std::vector<std::string>&) {
        return lit(static_cast<i64>(b_.frame_rate));
    });
    reg("Graphics", "width", 0, [this](const std::vector<std::string>&) {
        return lit(static_cast<i64>(b_.screen_width));
    });
    reg("Graphics", "height", 0, [this](const std::vector<std::string>&) {
        return lit(static_cast<i64>(b_.screen_height));
    });
    reg("Graphics", "fade_out", 1, [this](const std::vector<std::string>&) {
        fade_request_ = 1;
        return lit(true);
    });
    reg("Graphics", "fade_in", 1, [this](const std::vector<std::string>&) {
        fade_request_ = 2;
        return lit(true);
    });

    // --- Audio ---------------------------------------------------------------
    const auto play_fn = [this](audio::AudioChannel ch) {
        return [this, ch](const std::vector<std::string>& a) {
            if (!b_.audio || a.empty()) return std::string("nil");
            audio::PlayParams p;
            p.volume = static_cast<i32>(arg_i64(a, 1, 80));
            p.pitch = static_cast<i32>(arg_i64(a, 2, 100));
            switch (ch) {
            case audio::AudioChannel::Bgm: b_.audio->bgm_play(a[0], p); break;
            case audio::AudioChannel::Bgs: b_.audio->bgs_play(a[0], p); break;
            case audio::AudioChannel::Me: b_.audio->me_play(a[0], p); break;
            case audio::AudioChannel::Se: b_.audio->se_play(a[0], p); break;
            }
            return lit(true);
        };
    };
    const auto stop_fn = [this](audio::AudioChannel ch) {
        return [this, ch](const std::vector<std::string>& a) {
            if (!b_.audio) return std::string("nil");
            const i32 fade = static_cast<i32>(arg_i64(a, 0, 0));
            switch (ch) {
            case audio::AudioChannel::Bgm: b_.audio->bgm_stop(fade); break;
            case audio::AudioChannel::Bgs: b_.audio->bgs_stop(fade); break;
            case audio::AudioChannel::Me: b_.audio->me_stop(fade); break;
            case audio::AudioChannel::Se: b_.audio->se_stop(); break;
            }
            return lit(true);
        };
    };
    reg("Audio", "bgm_play", -1, play_fn(audio::AudioChannel::Bgm));
    reg("Audio", "bgs_play", -1, play_fn(audio::AudioChannel::Bgs));
    reg("Audio", "me_play", -1, play_fn(audio::AudioChannel::Me));
    reg("Audio", "se_play", -1, play_fn(audio::AudioChannel::Se));
    reg("Audio", "bgm_stop", -1, stop_fn(audio::AudioChannel::Bgm));
    reg("Audio", "bgs_stop", -1, stop_fn(audio::AudioChannel::Bgs));
    reg("Audio", "me_stop", -1, stop_fn(audio::AudioChannel::Me));
    reg("Audio", "se_stop", -1, stop_fn(audio::AudioChannel::Se));
    reg("Audio", "stop_all", 0, [this](const std::vector<std::string>&) {
        if (b_.audio) b_.audio->stop_all();
        return lit(true);
    });
    reg("Audio", "bgm_volume", 1, [this](const std::vector<std::string>& a) {
        if (b_.audio) {
            b_.audio->set_master_volume(audio::AudioChannel::Bgm,
                                        static_cast<i32>(arg_i64(a, 0, 80)));
        }
        return lit(true);
    });
    reg("Audio", "se_volume", 1, [this](const std::vector<std::string>& a) {
        if (b_.audio) {
            b_.audio->set_master_volume(audio::AudioChannel::Se,
                                        static_cast<i32>(arg_i64(a, 0, 80)));
        }
        return lit(true);
    });
    reg("Audio", "bgm_playing?", 0, [this](const std::vector<std::string>&) {
        return lit(b_.audio && b_.audio->bgm_state().playing);
    });

    // --- Input ---------------------------------------------------------------
    const auto action_name = [](std::string name) -> std::string {
        if (!name.empty() && name.front() == ':') name = name.substr(1);
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        if (name == "c") return "confirm";
        if (name == "b" || name == "x") return "cancel";
        if (name == "a" || name == "menu") return "menu";
        if (name == "up" || name == "down" || name == "left" || name == "right") return name;
        if (name == "confirm" || name == "cancel" || name == "menu") return name;
        return name; // unbekannt → direkt als Action-Name versuchen
    };
    const auto press = [&](bool trigger) {
        return [this, trigger, action_name](const std::vector<std::string>& a) {
            if (!b_.input || a.empty()) return std::string("false");
            const std::string act = action_name(a[0]);
            const bool v = trigger ? b_.input->was_pressed(act) : b_.input->is_down(act);
            return lit(v);
        };
    };
    reg("Input", "press?", 1, press(false));
    reg("Input", "trigger?", 1, press(true));
    reg("Input", "released?", 1, [this, action_name](const std::vector<std::string>& a) {
        if (!b_.input || a.empty()) return std::string("false");
        return lit(b_.input->was_released(action_name(a[0])));
    });
    reg("Input", "dir4", 0, [this](const std::vector<std::string>&) {
        if (!b_.input) return std::string("0");
        if (b_.input->is_down("up")) return std::string("8");
        if (b_.input->is_down("down")) return std::string("2");
        if (b_.input->is_down("left")) return std::string("4");
        if (b_.input->is_down("right")) return std::string("6");
        return std::string("0");
    });
    reg("Input", "dir8", 0, [this](const std::vector<std::string>&) {
        if (!b_.input) return std::string("0");
        const bool u = b_.input->is_down("up"), d = b_.input->is_down("down");
        const bool l = b_.input->is_down("left"), r = b_.input->is_down("right");
        if (u && r) return std::string("9");
        if (u && l) return std::string("7");
        if (d && r) return std::string("3");
        if (d && l) return std::string("1");
        if (u) return std::string("8");
        if (d) return std::string("2");
        if (l) return std::string("4");
        if (r) return std::string("6");
        return std::string("0");
    });

    // --- SceneManager --------------------------------------------------------
    reg("SceneManager", "goto", 1, [this](const std::vector<std::string>& a) {
        if (a.empty()) return std::string("nil");
        std::string scene = a[0];
        if (!scene.empty() && scene.front() == ':') scene = scene.substr(1);
        std::transform(scene.begin(), scene.end(), scene.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        if (scene == "exit" && b_.gctx) {
            b_.gctx->request_quit = true;
            return lit(true);
        }
        if (scene == "restart" && b_.gctx) {
            b_.gctx->request_new_game = true;
            return lit(true);
        }
        scene_request_ = scene;
        return lit(true);
    });
    reg("SceneManager", "scene", 0, [this](const std::vector<std::string>&) {
        if (!b_.scenes || !b_.scenes->current()) return std::string("nil");
        const auto id = b_.scenes->current()->id();
        const char* names[] = {"title", "map",  "menu",   "dialog",
                               "save",  "battle", "choice", "gameover"};
        const auto idx = static_cast<usize>(id);
        return idx < 8 ? lit_str(names[idx]) : std::string("nil");
    });

    // --- Player --------------------------------------------------------------
    reg("Player", "transfer", -1, [this](const std::vector<std::string>& a) {
        transfer_map_ = static_cast<u32>(arg_i64(a, 0, 1));
        transfer_pos_ = {static_cast<f32>(arg_f64(a, 1, 0.0)),
                         static_cast<f32>(arg_f64(a, 2, 0.0)),
                         static_cast<f32>(arg_f64(a, 3, 0.0))};
        transfer_dir_ = static_cast<i32>(arg_i64(a, 4, 2));
        transfer_pending_ = true;
        return lit(true);
    });
    reg("Player", "x", 0, [this](const std::vector<std::string>&) {
        return lit(static_cast<f64>(b_.player ? b_.player->position().x : 0.0f));
    });
    reg("Player", "y", 0, [this](const std::vector<std::string>&) {
        return lit(static_cast<f64>(b_.player ? b_.player->position().y : 0.0f));
    });
    reg("Player", "z", 0, [this](const std::vector<std::string>&) {
        return lit(static_cast<f64>(b_.player ? b_.player->position().z : 0.0f));
    });
    reg("Player", "facing", 0, [this](const std::vector<std::string>&) {
        return lit(static_cast<i64>(b_.player ? b_.player->facing() : 2));
    });
    reg("Player", "set_position", 3, [this](const std::vector<std::string>& a) {
        if (b_.player) {
            b_.player->set_position(
                {static_cast<f32>(arg_f64(a, 0, 0.0)),
                 static_cast<f32>(arg_f64(a, 1, 0.0)),
                 static_cast<f32>(arg_f64(a, 2, 0.0))});
        }
        return lit(true);
    });
    reg("Player", "move", 3, [this](const std::vector<std::string>& a) {
        if (b_.player) {
            const auto p = b_.player->position();
            b_.player->set_position(
                {p.x + static_cast<f32>(arg_f64(a, 0, 0.0)),
                 p.y + static_cast<f32>(arg_f64(a, 1, 0.0)),
                 p.z + static_cast<f32>(arg_f64(a, 2, 0.0))});
        }
        return lit(true);
    });

    // --- NPC ----------------------------------------------------------------
    const auto find_scene_object = [this](const std::string& name) -> scene::SceneObject* {
        if (!b_.scene) return nullptr;
        for (auto& o : b_.scene->objects()) {
            if (o.name == name) return &o;
        }
        return nullptr;
    };
    reg("NPC", "find", 1, [this, find_scene_object](const std::vector<std::string>& a) {
        if (a.empty()) return std::string("nil");
        auto* o = find_scene_object(a[0]);
        return o ? lit(static_cast<i64>(o->id)) : std::string("nil");
    });
    reg("NPC", "x", 1, [this, find_scene_object](const std::vector<std::string>& a) {
        if (a.empty()) return std::string("0");
        auto* o = find_scene_object(a[0]);
        return lit(static_cast<f64>(o ? o->transform.position.x : 0.0f));
    });
    reg("NPC", "y", 1, [this, find_scene_object](const std::vector<std::string>& a) {
        if (a.empty()) return std::string("0");
        auto* o = find_scene_object(a[0]);
        return lit(static_cast<f64>(o ? o->transform.position.y : 0.0f));
    });
    reg("NPC", "z", 1, [this, find_scene_object](const std::vector<std::string>& a) {
        if (a.empty()) return std::string("0");
        auto* o = find_scene_object(a[0]);
        return lit(static_cast<f64>(o ? o->transform.position.z : 0.0f));
    });
    reg("NPC", "set_position", 4,
        [this, find_scene_object](const std::vector<std::string>& a) {
            if (a.size() < 4) return std::string("false");
            auto* o = find_scene_object(a[0]);
            if (!o) return std::string("false");
            o->transform.position = {static_cast<f32>(arg_f64(a, 1, 0.0)),
                                     static_cast<f32>(arg_f64(a, 2, 0.0)),
                                     static_cast<f32>(arg_f64(a, 3, 0.0))};
            if (o->collision_id && b_.scene) {
                b_.scene->collision().set_transform(o->collision_id, o->transform);
            }
            return lit(true);
        });
    reg("NPC", "say", -1, [this, find_scene_object](const std::vector<std::string>& a) {
        if (a.size() < 2 || !b_.gctx) return std::string("false");
        auto* o = find_scene_object(a[0]);
        if (!o) return std::string("false");
        b_.gctx->dialog_lines = {a[1]};
        b_.gctx->dialog_index = 0;
        b_.gctx->dialog_open = true;
        if (b_.scenes && (!b_.scenes->current() ||
                          b_.scenes->current()->id() != game::GameSceneId::Dialog)) {
            b_.scenes->push(std::make_unique<game::DialogScene>(), *b_.gctx);
        }
        return lit(true);
    });

    // --- Enemy ---------------------------------------------------------------
    reg("Enemy", "spawn", -1, [this](const std::vector<std::string>& a) {
        if (!b_.scene || a.size() < 4 || !b_.database) return std::string("nil");
        const u32 id = static_cast<u32>(arg_i64(a, 0, 1));
        const auto* ed = b_.database->enemies.empty() ? nullptr : &b_.database->enemies.front();
        for (const auto& e : b_.database->enemies) {
            if (e.id == id) {
                ed = &e;
                break;
            }
        }
        if (!ed) return std::string("nil");
        render::Transform t;
        t.position = {static_cast<f32>(arg_f64(a, 1, 0.0)),
                      static_cast<f32>(arg_f64(a, 2, 0.0)),
                      static_cast<f32>(arg_f64(a, 3, 0.0))};
        t.scale = {0.6f, 1.2f, 0.6f};
        auto mesh = render::Mesh::create_cube(1.0f);
        const auto eid = b_.scene->place(scene::ObjectType::Enemy, "Enemy_" + std::to_string(id),
                                         mesh, t);
        if (auto* o = b_.scene->find(eid)) {
            o->material.albedo = render::Color{0.8f, 0.15f, 0.15f, 1.0f};
        }
        return lit(static_cast<i64>(eid));
    });
    reg("Enemy", "count", 0, [this](const std::vector<std::string>&) {
        if (!b_.scene) return std::string("0");
        i64 n = 0;
        for (const auto& o : b_.scene->objects()) {
            if (o.type == scene::ObjectType::Enemy) ++n;
        }
        return lit(n);
    });
    reg("Enemy", "kill", 1, [this](const std::vector<std::string>& a) {
        if (a.empty() || !b_.scene) return std::string("false");
        for (auto& o : b_.scene->objects()) {
            if (o.name == a[0]) {
                b_.scene->remove_object(o.id);
                return std::string("true");
            }
        }
        return std::string("false");
    });
    reg("Enemy", "alive?", 1, [this, find_scene_object](const std::vector<std::string>& a) {
        if (a.empty()) return std::string("false");
        return lit(find_scene_object(a[0]) != nullptr);
    });

    // --- Camera --------------------------------------------------------------
    reg("Camera", "move_to", 3, [this](const std::vector<std::string>& a) {
        if (b_.camera) {
            b_.camera->set_position({static_cast<f32>(arg_f64(a, 0, 0.0)),
                                     static_cast<f32>(arg_f64(a, 1, 0.0)),
                                     static_cast<f32>(arg_f64(a, 2, 0.0))});
        }
        return lit(true);
    });
    reg("Camera", "look_at", 3, [this](const std::vector<std::string>& a) {
        if (b_.camera) {
            b_.camera->set_target({static_cast<f32>(arg_f64(a, 0, 0.0)),
                                   static_cast<f32>(arg_f64(a, 1, 0.0)),
                                   static_cast<f32>(arg_f64(a, 2, 0.0))});
        }
        return lit(true);
    });
    reg("Camera", "zoom", 1, [this](const std::vector<std::string>& a) {
        if (b_.camera) {
            b_.camera->set_perspective(static_cast<f32>(arg_f64(a, 0, 45.0)),
                                       b_.camera->aspect(), b_.camera->near_plane(),
                                       b_.camera->far_plane());
        }
        return lit(true);
    });
    reg("Camera", "shake", 2, [this](const std::vector<std::string>& a) {
        shake_power_ = static_cast<f32>(arg_f64(a, 0, 0.0));
        shake_duration_ = static_cast<f32>(arg_f64(a, 1, 0.0));
        return lit(true);
    });
    reg("Camera", "reset", 0, [this](const std::vector<std::string>&) {
        if (b_.camera) {
            b_.camera->set_position({0.0f, 10.0f, 12.0f});
            b_.camera->set_target({0.0f, 0.0f, 0.0f});
        }
        return lit(true);
    });

    // --- Weather -------------------------------------------------------------
    reg("Weather", "set", -1, [this](const std::vector<std::string>& a) {
        if (!b_.weather) return std::string("nil");
        const auto type = a.empty() ? game::WeatherType::None
                                    : game::WeatherSystem::from_string(a[0]);
        const f32 power = static_cast<f32>(arg_f64(a, 1, 5.0));
        b_.weather->set(type, power, 0.5f);
        return lit(true);
    });
    reg("Weather", "clear", -1, [this](const std::vector<std::string>&) {
        if (b_.weather) b_.weather->clear(0.5f);
        return lit(true);
    });
    reg("Weather", "type", 0, [this](const std::vector<std::string>&) {
        if (!b_.weather) return std::string("nil");
        return lit_str(game::WeatherSystem::to_string(b_.weather->state().type));
    });
    reg("Weather", "power", 0, [this](const std::vector<std::string>&) {
        if (!b_.weather) return std::string("0");
        return lit(static_cast<f64>(b_.weather->state().power));
    });

    // --- Inventory -----------------------------------------------------------
    reg("Inventory", "gain", -1, [this](const std::vector<std::string>& a) {
        if (!b_.inventory || a.empty()) return std::string("false");
        const u32 id = resolve_item_id(a[0]);
        if (id == 0) return std::string("false");
        b_.inventory->gain_item(id, static_cast<i32>(arg_i64(a, 1, 1)));
        return lit(true);
    });
    reg("Inventory", "lose", -1, [this](const std::vector<std::string>& a) {
        if (!b_.inventory || a.empty()) return std::string("false");
        const u32 id = resolve_item_id(a[0]);
        if (id == 0) return std::string("false");
        return lit(b_.inventory->lose_item(id, static_cast<i32>(arg_i64(a, 1, 1))));
    });
    reg("Inventory", "count", 1, [this](const std::vector<std::string>& a) {
        if (!b_.inventory || a.empty()) return std::string("0");
        return lit(static_cast<i64>(b_.inventory->item_count(resolve_item_id(a[0]))));
    });
    reg("Inventory", "has?", 1, [this](const std::vector<std::string>& a) {
        if (!b_.inventory || a.empty()) return std::string("false");
        return lit(b_.inventory->item_count(resolve_item_id(a[0])) > 0);
    });
    reg("Inventory", "gold", 0, [this](const std::vector<std::string>&) {
        return lit(static_cast<i64>(b_.inventory ? b_.inventory->gold() : 0));
    });
    reg("Inventory", "gold=", 1, [this](const std::vector<std::string>& a) {
        if (b_.inventory) b_.inventory->set_gold(static_cast<i32>(arg_i64(a, 0, 0)));
        return lit(true);
    });
    reg("Inventory", "items", 0, [this](const std::vector<std::string>&) {
        if (!b_.inventory) return std::string("\"\"");
        std::string out;
        for (const auto& it : b_.inventory->items()) {
            if (!out.empty()) out += ";";
            out += std::to_string(it.item_id) + ":" + std::to_string(it.count);
        }
        return lit_str(out);
    });

    // --- Quest ---------------------------------------------------------------
    reg("Quest", "start", 1, [this](const std::vector<std::string>& a) {
        if (!b_.quests || a.empty()) return std::string("false");
        return lit(b_.quests->start(a[0]));
    });
    reg("Quest", "complete", 1, [this](const std::vector<std::string>& a) {
        if (!b_.quests || a.empty()) return std::string("false");
        return lit(b_.quests->complete(a[0]));
    });
    reg("Quest", "fail", 1, [this](const std::vector<std::string>& a) {
        if (!b_.quests || a.empty()) return std::string("false");
        return lit(b_.quests->fail(a[0]));
    });
    reg("Quest", "advance", -1, [this](const std::vector<std::string>& a) {
        if (!b_.quests || a.empty()) return std::string("false");
        return lit(b_.quests->advance(a[0], static_cast<i32>(arg_i64(a, 1, 1))));
    });
    reg("Quest", "active?", 1, [this](const std::vector<std::string>& a) {
        if (!b_.quests || a.empty()) return std::string("false");
        return lit(b_.quests->active(a[0]));
    });
    reg("Quest", "completed?", 1, [this](const std::vector<std::string>& a) {
        if (!b_.quests || a.empty()) return std::string("false");
        return lit(b_.quests->completed(a[0]));
    });
    reg("Quest", "list", 0, [this](const std::vector<std::string>&) {
        if (!b_.quests) return std::string("\"\"");
        std::string out;
        for (const auto& q : b_.quests->all()) {
            if (!out.empty()) out += ";";
            out += q.id + ":" + std::to_string(static_cast<int>(q.status));
        }
        return lit_str(out);
    });

    // --- Dialogue ------------------------------------------------------------
    reg("Dialogue", "start", 1, [this](const std::vector<std::string>& a) {
        if (!b_.gctx || a.empty()) return std::string("false");
        b_.gctx->dialog_lines = {a[0]};
        b_.gctx->dialog_index = 0;
        b_.gctx->dialog_open = true;
        if (b_.scenes && (!b_.scenes->current() ||
                          b_.scenes->current()->id() != game::GameSceneId::Dialog)) {
            b_.scenes->push(std::make_unique<game::DialogScene>(), *b_.gctx);
        }
        return lit(true);
    });
    reg("Dialogue", "text", 0, [this](const std::vector<std::string>&) {
        if (!b_.gctx || b_.gctx->dialog_lines.empty()) return std::string("\"\"");
        return lit_str(b_.gctx->dialog_lines[static_cast<usize>(b_.gctx->dialog_index)]);
    });
    reg("Dialogue", "choices", -1, [this](const std::vector<std::string>& a) {
        if (!b_.gctx || a.empty()) return std::string("false");
        b_.gctx->choice_labels = a;
        b_.gctx->choice_index = 0;
        b_.gctx->choice_result = -1;
        b_.gctx->choice_open = true;
        if (b_.scenes && (!b_.scenes->current() ||
                          b_.scenes->current()->id() != game::GameSceneId::Choice)) {
            b_.scenes->push(std::make_unique<game::ChoiceScene>(), *b_.gctx);
        }
        return lit(true);
    });
    reg("Dialogue", "choice", 0, [this](const std::vector<std::string>&) {
        if (!b_.gctx) return std::string("-1");
        return lit(static_cast<i64>(b_.gctx->choice_result));
    });
    reg("Dialogue", "close", 0, [this](const std::vector<std::string>&) {
        if (b_.gctx) {
            b_.gctx->dialog_open = false;
            b_.gctx->choice_open = false;
        }
        return lit(true);
    });

    // --- Map -----------------------------------------------------------------
    reg("Map", "id", 0, [this](const std::vector<std::string>&) {
        return lit(static_cast<i64>(b_.map_id));
    });
    reg("Map", "name", 0, [this](const std::vector<std::string>&) {
        return lit_str(b_.map_name);
    });
    reg("Map", "load", 1, [this](const std::vector<std::string>& a) {
        map_load_id_ = static_cast<u32>(arg_i64(a, 0, 1));
        map_load_pending_ = true;
        return lit(true);
    });
    reg("Map", "width", 0, [this](const std::vector<std::string>&) {
        return lit(static_cast<i64>(b_.map_width));
    });
    reg("Map", "height", 0, [this](const std::vector<std::string>&) {
        return lit(static_cast<i64>(b_.map_height));
    });
    reg("Map", "tint", 4, [this](const std::vector<std::string>& a) {
        tint_ = {static_cast<f32>(arg_f64(a, 0, 0.0)),
                 static_cast<f32>(arg_f64(a, 1, 0.0)),
                 static_cast<f32>(arg_f64(a, 2, 0.0)),
                 static_cast<f32>(arg_f64(a, 3, 0.0))};
        return lit(true);
    });

    // --- Game (Schalter/Variablen – Kompatibilität) --------------------------
    reg("Game", "switch", 1, [this](const std::vector<std::string>& a) {
        if (!b_.state || a.empty()) return std::string("false");
        return lit(b_.state->get_switch(static_cast<u32>(arg_i64(a, 0, 1))));
    });
    reg("Game", "set_switch", 2, [this](const std::vector<std::string>& a) {
        if (b_.state && a.size() >= 2) {
            const bool v = a[1] == "true" || a[1] == "1";
            b_.state->set_switch(static_cast<u32>(arg_i64(a, 0, 1)), v);
        }
        return lit(true);
    });
    reg("Game", "variable", 1, [this](const std::vector<std::string>& a) {
        if (!b_.state || a.empty()) return std::string("0");
        return lit(static_cast<i64>(b_.state->get_variable(static_cast<u32>(arg_i64(a, 0, 1)))));
    });
    reg("Game", "set_variable", 2, [this](const std::vector<std::string>& a) {
        if (b_.state && a.size() >= 2) {
            b_.state->set_variable(static_cast<u32>(arg_i64(a, 0, 1)),
                                   static_cast<i32>(arg_i64(a, 1, 0)));
        }
        return lit(true);
    });

    core::log_info("Ruby", "RubyHost: engine API bound to live systems");
}

} // namespace aether::ruby
