/**
 * @file bootstrap.cpp
 * @brief Game-Runtime mit Titel, Map, Menü, Dialog, Save/Load.
 */
#include <aether/runtime/bootstrap.hpp>

#include <aether/aether.hpp>
#include <aether/anim/animation.hpp>
#include <aether/game/battle.hpp>
#include <aether/game/inventory.hpp>
#include <aether/game/map_loader.hpp>
#include <aether/game/player.hpp>
#include <aether/game/quest.hpp>
#include <aether/game/save_system.hpp>
#include <aether/game/scene_stack.hpp>
#include <aether/game/shop.hpp>
#include <aether/game/ui_hud.hpp>
#include <aether/game/weather.hpp>
#include <aether/shared/project_descriptor.hpp>

#include <iostream>
#include <string>

namespace aether::runtime {
namespace {

core::EngineConfig make_engine_config(const shared::ProjectDescriptor& project,
                                      const RuntimeOptions& opt) {
    core::EngineConfig cfg;
    cfg.mode = opt.headless ? core::AppMode::Headless : core::AppMode::Runtime;
    cfg.graphics.title =
        project.graphics.title.empty() ? project.name : project.graphics.title;
    cfg.graphics.width = project.graphics.width;
    cfg.graphics.height = project.graphics.height;
    cfg.graphics.fullscreen = project.graphics.fullscreen;
    cfg.graphics.vsync = project.graphics.vsync;
    cfg.graphics.frame_rate = project.graphics.frame_rate;
    cfg.audio.bgm_volume = project.audio.bgm_volume;
    cfg.audio.bgs_volume = project.audio.bgs_volume;
    cfg.audio.me_volume = project.audio.me_volume;
    cfg.audio.se_volume = project.audio.se_volume;
    cfg.paths.project_dir = project.root_dir;
    cfg.paths.logs_dir = project.root_dir / "logs";
    cfg.enable_ruby = opt.enable_ruby;
    cfg.log.level = opt.headless ? "warn" : "info";
    cfg.log.console = true;
    cfg.log.file = !opt.headless;
    return cfg;
}

EntityId find_player(scene::Scene& sc) {
    for (const auto& o : sc.objects()) {
        if (o.type == scene::ObjectType::Character || o.name == "Player") {
            return o.id;
        }
    }
    return kInvalidEntity;
}

struct RuntimeState {
    shared::ProjectDescriptor project;
    std::unique_ptr<scene::Scene> scene;
    game::Database database;
    game::PlayerController player;
    game::FollowCamera follow_cam;
    game::GameState game_state;
    game::EventInterpreter interpreter{&game_state};
    game::WeatherSystem weather;
    game::PartyInventory inventory;
    game::SaveSystem saves;
    game::QuestLog quests;
    game::Battle battle;
    bool in_battle = false;
    game::HudBuilder hud;
    game::GameSceneStack scenes;
    game::GameContext gctx;
    render::Camera camera;
    u32 map_id = 1;
    i32 playtime = 0;
    f64 playtime_accum = 0.0;
    bool map_active = false;
    std::unique_ptr<ruby::RubyVM> vm;
};

void open_dialog(RuntimeState& rs, std::vector<std::string> lines) {
    rs.gctx.dialog_lines = std::move(lines);
    rs.gctx.dialog_index = 0;
    rs.gctx.dialog_open = true;
    rs.scenes.push(std::make_unique<game::DialogScene>(), rs.gctx);
}

void bind_ruby(RuntimeState& rs, audio::AudioEngine& audio) {
    if (!rs.vm) {
        return;
    }
    auto& vm = *rs.vm;
    audio::AudioEngine* ap = &audio;
    game::WeatherSystem* wp = &rs.weather;
    game::GameState* sp = &rs.game_state;
    game::PlayerController* pp = &rs.player;
    game::PartyInventory* inv = &rs.inventory;

    vm.define_function(
        {"Audio", "bgm_play", -1, [ap](const std::vector<std::string>& args) {
             if (!args.empty()) {
                 audio::PlayParams p;
                 if (args.size() > 1) p.volume = std::stoi(args[1]);
                 if (args.size() > 2) p.pitch = std::stoi(args[2]);
                 ap->bgm_play(args[0], p);
             }
             return std::string("nil");
         }});
    vm.define_function(
        {"Audio", "se_play", -1, [ap](const std::vector<std::string>& args) {
             if (!args.empty()) {
                 audio::PlayParams p;
                 if (args.size() > 1) p.volume = std::stoi(args[1]);
                 ap->se_play(args[0], p);
             }
             return std::string("nil");
         }});
    vm.define_function(
        {"Weather", "set", -1, [wp](const std::vector<std::string>& args) {
             const auto type = args.empty() ? game::WeatherType::None
                                            : game::WeatherSystem::from_string(args[0]);
             const f32 power = args.size() > 1 ? std::stof(args[1]) : 5.0f;
             wp->set(type, power, 0.5f);
             return std::string("nil");
         }});
    vm.define_function({"Player", "x", 0, [pp](const std::vector<std::string>&) {
                            return std::to_string(pp->position().x);
                        }});
    vm.define_function({"Player", "z", 0, [pp](const std::vector<std::string>&) {
                            return std::to_string(pp->position().z);
                        }});
    vm.define_function(
        {"Inventory", "gain", -1, [inv](const std::vector<std::string>& args) {
             if (args.size() >= 1) {
                 // name or id – try id
                 try {
                     const u32 id = static_cast<u32>(std::stoul(args[0]));
                     const i32 n = args.size() > 1 ? std::stoi(args[1]) : 1;
                     inv->gain_item(id, n);
                 } catch (...) {
                 }
             }
             return std::string("nil");
         }});
    vm.define_function(
        {"Inventory", "gold", 0, [inv](const std::vector<std::string>&) {
             return std::to_string(inv->gold());
         }});
    vm.define_function(
        {"Game", "switch", 1, [sp](const std::vector<std::string>& args) -> std::string {
             if (args.empty()) return "false";
             return sp->get_switch(static_cast<u32>(std::stoul(args[0]))) ? "true"
                                                                          : "false";
         }});
    vm.define_function(
        {"Game", "set_switch", 2, [sp](const std::vector<std::string>& args) {
             if (args.size() >= 2) {
                 sp->set_switch(static_cast<u32>(std::stoul(args[0])),
                                args[1] == "true" || args[1] == "1");
             }
             return std::string("nil");
         }});
}

bool load_map_into(RuntimeState& rs, render::Renderer* renderer, u32 map_id,
                   const render::Vec3* override_pos) {
    const auto map_file =
        game::map_path_for_id(rs.project.root_dir / rs.project.maps_path, map_id);
    auto map = game::load_map(map_file, nullptr, renderer);
    if (!map.scene) {
        return false;
    }
    rs.scene = std::move(map.scene);
    rs.map_id = map_id;

    EntityId player_id = find_player(*rs.scene);
    if (player_id == kInvalidEntity) {
        render::Transform pt;
        pt.position = override_pos ? *override_pos
                                   : render::Vec3{static_cast<f32>(rs.project.start.x), 0.f,
                                                  static_cast<f32>(rs.project.start.z)};
        pt.scale = {0.6f, 1.2f, 0.6f};
        auto mesh = render::Mesh::create_cube(1.0f);
        if (renderer) renderer->upload_mesh(*mesh);
        player_id = rs.scene->place(scene::ObjectType::Character, "Player", mesh, pt);
        if (auto* p = rs.scene->find(player_id)) {
            p->material.albedo = render::Color{0.2f, 0.55f, 1.0f, 1.0f};
        }
    }

    u32 col = 0;
    if (auto* p = rs.scene->find(player_id)) {
        col = p->collision_id;
        if (override_pos) {
            p->transform.position = *override_pos;
            if (col) {
                rs.scene->collision().set_transform(col, p->transform);
            }
        }
    }
    rs.player.bind(rs.scene.get(), player_id, col);
    if (override_pos) {
        rs.player.set_position(*override_pos);
    } else if (auto* p = rs.scene->find(player_id)) {
        rs.player.set_position(p->transform.position);
    }
    rs.follow_cam.snap(rs.player.position());
    rs.map_active = true;
    return true;
}

void handle_event_requests(RuntimeState& rs);

void start_new_game(RuntimeState& rs, render::Renderer* renderer) {
    rs.game_state.clear();
    rs.inventory.setup_from_database(rs.database);
    rs.weather.clear(0.0f);
    rs.quests = game::QuestLog{};
    rs.quests.load_defaults();
    rs.in_battle = false;
    rs.playtime = 0;
    const render::Vec3 start{static_cast<f32>(rs.project.start.x), 0.f,
                             static_cast<f32>(rs.project.start.z)};
    load_map_into(rs, renderer, static_cast<u32>(rs.project.start.map_id), &start);
    rs.scenes.clear();
    rs.scenes.push(std::make_unique<game::MapScene>(), rs.gctx);
    rs.gctx.status_line = "Neues Spiel";
    if (!rs.database.items.empty()) {
        rs.inventory.gain_item(rs.database.items.front().id, 3);
    }
    rs.quests.start("main_001");
}

void start_battle(RuntimeState& rs, u32 enemy_id) {
    if (rs.inventory.party().empty() || rs.database.enemies.empty()) {
        return;
    }
    const game::EnemyData* ed = &rs.database.enemies.front();
    for (const auto& e : rs.database.enemies) {
        if (e.id == enemy_id) {
            ed = &e;
            break;
        }
    }
    const game::SkillData* sk =
        rs.database.skills.empty() ? nullptr : &rs.database.skills.front();
    rs.battle.start(rs.inventory.party().front(), *ed, sk);
    rs.battle.set_log([](const std::string& s) { core::log_info("Battle", s); });
    rs.in_battle = true;

    auto bs = std::make_unique<game::BattleScene>();
    bs->set_tick([&rs](game::GameContext& ctx) {
        if (!rs.in_battle || !ctx.input) {
            return;
        }
        rs.battle.update(*ctx.input);
        ctx.status_line = rs.battle.status_line();
        if (rs.battle.finished()) {
            if (ctx.input->was_pressed("confirm") || ctx.input->was_pressed("cancel") ||
                ctx.battle_done_ack) {
                const auto res = rs.battle.result();
                if (res.won) {
                    rs.inventory.gain_exp(res.exp);
                    rs.inventory.gain_gold(res.gold);
                    // sync party HP from battle
                    if (!rs.inventory.party().empty()) {
                        rs.inventory.party()[0].hp = rs.battle.hero().hp;
                        rs.inventory.party()[0].mp = rs.battle.hero().mp;
                    }
                    rs.quests.advance("hunt_001");
                    rs.quests.complete("hunt_001");
                } else if (!res.fled && !rs.inventory.party().empty()) {
                    rs.inventory.party()[0].hp = 1; // soft fail
                }
                rs.in_battle = false;
                rs.scenes.pop(ctx);
            }
        }
    });
    rs.scenes.push(std::move(bs), rs.gctx);
}

void handle_event_requests(RuntimeState& rs) {
    const auto& req = rs.interpreter.pending_request();
    if (req.kind == game::EventRequest::Kind::None) {
        return;
    }
    switch (req.kind) {
    case game::EventRequest::Kind::Choice:
        rs.gctx.choice_labels = req.choice_labels;
        rs.gctx.choice_index = 0;
        rs.gctx.choice_result = -1;
        rs.gctx.choice_open = true;
        rs.scenes.push(std::make_unique<game::ChoiceScene>(), rs.gctx);
        break;
    case game::EventRequest::Kind::Weather: {
        const auto type =
            game::WeatherSystem::from_string(req.params.value("type", "rain"));
        const f32 power = req.params.value("power", 5.0f);
        rs.weather.set(type, power, req.params.value("frames", 0) / 60.0f);
        rs.interpreter.clear_pending();
        break;
    }
    case game::EventRequest::Kind::QuestStart:
        rs.quests.start(req.params.value("id", ""));
        rs.interpreter.clear_pending();
        break;
    case game::EventRequest::Kind::QuestComplete: {
        const std::string id = req.params.value("id", "");
        if (rs.quests.complete(id)) {
            if (const auto* def = rs.quests.find_def(id)) {
                rs.inventory.gain_exp(def->exp_reward);
                rs.inventory.gain_gold(def->gold_reward);
                if (def->item_reward_id && def->item_reward_count > 0) {
                    rs.inventory.gain_item(def->item_reward_id, def->item_reward_count);
                }
            }
        }
        rs.interpreter.clear_pending();
        break;
    }
    case game::EventRequest::Kind::Shop: {
        game::Shop shop;
        shop.set_name(req.params.value("name", "Händler"));
        std::vector<game::ShopOffer> offers;
        if (req.params.contains("items") && req.params["items"].is_array()) {
            for (const auto& it : req.params["items"]) {
                offers.push_back({it.get<u32>(), -1});
            }
        } else if (!rs.database.items.empty()) {
            offers.push_back({rs.database.items.front().id, -1});
        }
        shop.set_offers(offers);
        // auto-buy first for headless; interactive would need UI – open as dialog list
        std::vector<std::string> lines;
        lines.push_back(shop.name() + " – Gold: " + std::to_string(rs.inventory.gold()));
        for (usize i = 0; i < shop.offers().size(); ++i) {
            const auto& o = shop.offers()[i];
            std::string name = "Item#" + std::to_string(o.item_id);
            for (const auto& d : rs.database.items) {
                if (d.id == o.item_id) {
                    name = d.name;
                    break;
                }
            }
            lines.push_back(name + " – " + std::to_string(shop.price_of(o, rs.database)) +
                            " G");
        }
        // buy first offer if affordable
        if (!offers.empty()) {
            shop.buy(0, rs.inventory, rs.database);
            lines.push_back("(Gekauft: erstes Angebot)");
        }
        open_dialog(rs, std::move(lines));
        rs.interpreter.clear_pending();
        break;
    }
    case game::EventRequest::Kind::Battle: {
        const u32 eid = req.params.value("enemy_id", 1u);
        rs.interpreter.clear_pending();
        start_battle(rs, eid);
        break;
    }
    case game::EventRequest::Kind::Camera:
        // soft: snap follow camera offset
        rs.follow_cam.set_offsets(req.params.value("height", 10.0f),
                                  req.params.value("back", 12.0f));
        rs.interpreter.clear_pending();
        break;
    default:
        rs.interpreter.clear_pending();
        break;
    }
}

void do_save(RuntimeState& rs, int slot) {
    if (!rs.scene) {
        return;
    }
    auto data = game::SaveSystem::capture(
        rs.project.graphics.title, rs.map_id, rs.scene->name(), rs.player.position(),
        rs.player.facing(), rs.inventory, rs.game_state, rs.weather, rs.playtime);
    auto r = rs.saves.save(slot, data);
    rs.gctx.status_line = r ? ("Gespeichert Slot " + std::to_string(slot))
                            : r.error().what();
    core::log_info("Runtime", rs.gctx.status_line);
}

bool do_load(RuntimeState& rs, render::Renderer* renderer, int slot) {
    auto loaded = rs.saves.load(slot);
    if (!loaded) {
        rs.gctx.status_line = loaded.error().what();
        return false;
    }
    const auto& d = loaded.value();
    game::SaveSystem::apply_state(d, rs.game_state, rs.inventory, rs.weather);
    rs.playtime = d.header.playtime_seconds;
    if (!load_map_into(rs, renderer, d.map_id, &d.player_pos)) {
        return false;
    }
    rs.scenes.clear();
    rs.scenes.push(std::make_unique<game::MapScene>(), rs.gctx);
    rs.gctx.status_line = "Geladen Slot " + std::to_string(slot);
    return true;
}

void push_menu(RuntimeState& rs, render::Renderer* renderer) {
    auto menu = std::make_unique<game::MenuScene>();
    menu->set_callback([&rs, renderer](const std::string& action) {
        if (action == "close" || action == "Weiterspielen") {
            rs.scenes.pop(rs.gctx);
            return;
        }
        if (action == "Beenden") {
            rs.gctx.request_quit = true;
            return;
        }
        if (action == "Titel") {
            rs.scenes.replace(std::make_unique<game::TitleScene>(), rs.gctx);
            rs.map_active = false;
            return;
        }
        if (action == "Items") {
            std::vector<std::string> lines;
            lines.push_back("Gold: " + std::to_string(rs.inventory.gold()));
            if (rs.inventory.items().empty()) {
                lines.push_back("(keine Items)");
            } else {
                for (const auto& it : rs.inventory.items()) {
                    std::string name = "Item#" + std::to_string(it.item_id);
                    for (const auto& d : rs.database.items) {
                        if (d.id == it.item_id) {
                            name = d.name;
                            break;
                        }
                    }
                    lines.push_back(name + " x" + std::to_string(it.count));
                }
            }
            if (!rs.inventory.party().empty()) {
                const auto& m = rs.inventory.party().front();
                lines.push_back(m.name + " Lv" + std::to_string(m.level) + " HP " +
                                std::to_string(m.hp) + "/" + std::to_string(m.max_hp));
            }
            // close menu first then dialog
            rs.scenes.pop(rs.gctx);
            open_dialog(rs, std::move(lines));
            return;
        }
        if (action == "Speichern") {
            rs.gctx.save_mode = true;
            rs.scenes.pop(rs.gctx);
            auto sl = std::make_unique<game::SaveLoadScene>();
            sl->set_callback([&rs](int slot, bool is_save) {
                rs.scenes.pop(rs.gctx);
                if (slot > 0 && is_save) {
                    do_save(rs, slot);
                }
            });
            rs.scenes.push(std::move(sl), rs.gctx);
            return;
        }
        if (action == "Laden") {
            rs.gctx.save_mode = false;
            rs.scenes.pop(rs.gctx);
            auto sl = std::make_unique<game::SaveLoadScene>();
            sl->set_callback([&rs, renderer](int slot, bool is_save) {
                rs.scenes.pop(rs.gctx);
                if (slot > 0 && !is_save) {
                    do_load(rs, renderer, slot);
                }
            });
            rs.scenes.push(std::move(sl), rs.gctx);
            return;
        }
    });
    rs.scenes.push(std::move(menu), rs.gctx);
}

void pump_interpreter(RuntimeState& rs) {
    if (!rs.interpreter.is_running()) {
        return;
    }
    if (rs.interpreter.is_waiting_for_choice()) {
        return;
    }
    rs.interpreter.update();
    if (!rs.interpreter.messages().empty() && !rs.gctx.dialog_open) {
        auto msgs = rs.interpreter.messages();
        rs.interpreter.clear_messages();
        open_dialog(rs, std::move(msgs));
        return;
    }
    if (rs.interpreter.pending_request().kind != game::EventRequest::Kind::None) {
        handle_event_requests(rs);
    }
}

void update_map_gameplay(RuntimeState& rs, input::InputManager& input, f64 fixed_dt) {
    if (!rs.map_active || !rs.scene) {
        return;
    }
    if (rs.in_battle) {
        return;
    }
    // Don't move while overlays open
    if (rs.scenes.current() && rs.scenes.current()->id() != game::GameSceneId::Map) {
        return;
    }

    if (rs.interpreter.is_running()) {
        pump_interpreter(rs);
        return;
    }

    rs.player.update_movement(input, fixed_dt, &rs.scene->collision());

    if (input.was_pressed("confirm")) {
        const EntityId target = rs.player.find_interact_target(*rs.scene);
        if (target != kInvalidEntity) {
            if (auto* obj = rs.scene->find(target);
                obj && obj->map_event && !obj->map_event->pages.empty()) {
                rs.interpreter.set_script_handler([&](const std::string& code) {
                    if (rs.vm) {
                        auto r = rs.vm->eval(code);
                        if (!r.ok) {
                            core::log_warn("Event", r.error);
                        }
                    }
                });
                rs.interpreter.set_transfer_handler(
                    [&](u32 map_id, f32 x, f32 y, f32 z, i32) {
                        const render::Vec3 pos{x, y, z};
                        load_map_into(rs, rs.gctx.renderer, map_id, &pos);
                    });
                rs.interpreter.start(obj->map_event->pages[0].commands);
                pump_interpreter(rs);
            }
        }
    }

    // Debug/demo: F-key via page_down starts hunt battle if enemy nearby flag
    if (input.was_pressed("page_down") && !rs.database.enemies.empty()) {
        rs.quests.start("hunt_001");
        start_battle(rs, rs.database.enemies.front().id);
    }

    if (input.was_pressed("menu")) {
        push_menu(rs, rs.gctx.renderer);
    }
}

} // namespace

RuntimeOptions parse_args(int argc, char** argv) {
    RuntimeOptions opt;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if ((a == "--project" || a == "-p") && i + 1 < argc) {
            opt.project_path = argv[++i];
        } else if (a == "--headless") {
            opt.headless = true;
        } else if ((a == "--max-frames") && i + 1 < argc) {
            opt.max_frames = std::stoi(argv[++i]);
        } else if (a == "--no-ruby") {
            opt.enable_ruby = false;
        } else if (a == "--help" || a == "-h") {
            std::cout
                << "AetherRPG Game Runtime\n"
                << "Usage: Game [options]\n"
                << "  -p, --project <path>   Project folder or project.json\n"
                << "  --headless             No window (CI)\n"
                << "  --max-frames <n>       Exit after n frames\n"
                << "  --no-ruby              Skip script boot\n"
                << "  -h, --help             Show help\n";
            opt.max_frames = 0;
        } else if (!a.empty() && a[0] != '-') {
            opt.project_path = a;
        }
    }
    return opt;
}

int run_game(const RuntimeOptions& options) {
    if (options.max_frames == 0) {
        return 0;
    }

    RuntimeState rs;
    std::string err;
    if (!shared::load_project_descriptor(options.project_path, rs.project, &err)) {
        std::cerr << "Failed to load project: " << err << '\n';
        return 1;
    }

    core::log_info("Runtime", "Loading project '" + rs.project.name + "' from " +
                                  rs.project.root_dir.string());

    auto ctx = core::EngineContext::create(make_engine_config(rs.project, options));
    ctx->start();

    rs.saves = game::SaveSystem(rs.project.root_dir / "saves");

    res::ResourceManager resources(&ctx->thread_pool());
    resources.mount("data", rs.project.root_dir / rs.project.data_path);
    resources.mount("maps", rs.project.root_dir / rs.project.maps_path);
    resources.mount("graphics", rs.project.root_dir / rs.project.graphics_path);
    resources.mount("audio", rs.project.root_dir / rs.project.audio_path);
    resources.mount("scripts", rs.project.root_dir / "scripts");

    auto db_res =
        game::Database::load_from_directory(rs.project.root_dir / rs.project.data_path);
    rs.database = db_res ? std::move(db_res.value()) : game::Database::make_default();

    window::WindowDesc wdesc = window::Window::desc_from_graphics(ctx->config().graphics);
    auto window = window::Window::create(
        wdesc, &ctx->events(),
        options.headless ? std::optional(window::WindowBackend::Null) : std::nullopt);

    render::RendererDesc rdesc;
    rdesc.backend = (window && window->backend() == window::WindowBackend::Glfw)
                        ? render::RendererBackend::OpenGL
                        : render::RendererBackend::Null;
    rdesc.clear_color = render::Color::cornflower();
    auto renderer = render::Renderer::create(rdesc, window.get());

    input::InputManager input(&ctx->events());
    input.register_default_rpg_actions();
    if (window) {
        input.attach_window(window.get());
    }

    auto audio = audio::AudioEngine::create(
#if defined(AETHER_WITH_MINIAUDIO)
        options.headless ? audio::AudioBackend::Null : audio::AudioBackend::MiniAudio,
#else
        audio::AudioBackend::Null,
#endif
        ctx->config().audio);

    // Register audio clips from project folders (name without extension)
    {
        namespace fs = std::filesystem;
        const char* sub[] = {"bgm", "bgs", "me", "se"};
        for (const char* s : sub) {
            const fs::path dir = rs.project.root_dir / rs.project.audio_path / s;
            std::error_code ec;
            if (!fs::exists(dir, ec)) continue;
            for (const auto& ent : fs::directory_iterator(dir, ec)) {
                if (!ent.is_regular_file()) continue;
                const auto ext = ent.path().extension().string();
                if (ext != ".wav" && ext != ".ogg" && ext != ".mp3" && ext != ".WAV") continue;
                audio->register_clip(ent.path().stem().string(), ent.path().string());
            }
        }
    }

    const f32 aspect = static_cast<f32>(wdesc.width) /
                       static_cast<f32>(wdesc.height > 0 ? wdesc.height : 1);
    rs.camera.set_perspective(45.0f, aspect, 0.1f, 500.0f);

    // Ruby / plugins
    plugin::PluginLoader plugins;
    const auto plug_count = plugins.scan(rs.project.root_dir / "plugins");
    if (options.enable_ruby) {
        rs.vm = ruby::RubyVM::create();
        rs.vm->define_engine_api();
        bind_ruby(rs, *audio);
        auto result = rs.vm->load_file((rs.project.root_dir / rs.project.scripts.entry).string());
        if (!result.ok) {
            core::log_warn("Runtime", "Script: " + result.error);
        }
        if (plug_count > 0) {
            (void)plugins.activate_all(*rs.vm);
        }
    }

    rs.gctx.input = &input;
    rs.gctx.renderer = renderer.get();

    // Start at title (headless CI: auto new game for smoke)
    if (options.headless && options.max_frames > 0) {
        start_new_game(rs, renderer.get());
    } else {
        rs.scenes.push(std::make_unique<game::TitleScene>(), rs.gctx);
    }

    if (window) {
        ctx->events().subscribe<window::WindowFramebufferResizeEvent>(
            [&](const window::WindowFramebufferResizeEvent& e) {
                if (renderer && e.width > 0 && e.height > 0) {
                    renderer->set_viewport(0, 0, e.width, e.height);
                    rs.camera.set_perspective(
                        45.0f, static_cast<f32>(e.width) / static_cast<f32>(e.height), 0.1f,
                        500.0f);
                }
            });
    }

    core::log_info("Runtime", "Entering main loop");
    i64 frames = 0;
    bool running = true;
    ctx->events().subscribe<window::WindowCloseEvent>(
        [&](const window::WindowCloseEvent&) { running = false; });

    const render::Color base_clear = render::Color::cornflower();

    while (running) {
        if (window) {
            window->poll_events();
            if (window->should_close()) {
                running = false;
            }
        }

        input.begin_frame();
        if (!ctx->pump_frame()) {
            break;
        }

        const f64 dt = ctx->time().delta_seconds();
        rs.gctx.delta = dt;
        audio->update(dt);
        rs.weather.update(dt);
        rs.playtime_accum += dt;
        while (rs.playtime_accum >= 1.0) {
            rs.playtime_accum -= 1.0;
            ++rs.playtime;
        }

        // Scene stack UI input
        rs.scenes.update(rs.gctx);

        // Title actions
        if (rs.gctx.request_new_game) {
            rs.gctx.request_new_game = false;
            start_new_game(rs, renderer.get());
        }
        if (rs.gctx.request_continue) {
            rs.gctx.request_continue = false;
            // open load scene from title
            rs.gctx.save_mode = false;
            auto sl = std::make_unique<game::SaveLoadScene>();
            sl->set_callback([&](int slot, bool is_save) {
                rs.scenes.pop(rs.gctx);
                if (slot > 0 && !is_save) {
                    if (!do_load(rs, renderer.get(), slot)) {
                        // back to title if fail
                        if (!rs.map_active) {
                            rs.scenes.replace(std::make_unique<game::TitleScene>(), rs.gctx);
                        }
                    }
                } else if (slot < 0 && !rs.map_active) {
                    rs.scenes.replace(std::make_unique<game::TitleScene>(), rs.gctx);
                }
            });
            // if currently title, push load on top
            rs.scenes.push(std::move(sl), rs.gctx);
        }
        if (rs.gctx.request_quit) {
            running = false;
        }

        // Dialog finished → pop & resume interpreter
        if (rs.scenes.current() &&
            rs.scenes.current()->id() == game::GameSceneId::Dialog && !rs.gctx.dialog_open) {
            rs.scenes.pop(rs.gctx);
            if (rs.interpreter.is_running()) {
                pump_interpreter(rs);
            }
        }

        // Choice finished → resume interpreter
        if (rs.scenes.current() &&
            rs.scenes.current()->id() == game::GameSceneId::Choice && !rs.gctx.choice_open) {
            const int result = rs.gctx.choice_result;
            rs.scenes.pop(rs.gctx);
            if (rs.interpreter.is_waiting_for_choice()) {
                rs.interpreter.resume_choice(result);
                pump_interpreter(rs);
            }
        }

        ctx->time().drain_fixed_steps([&](f64 fixed_dt) {
            rs.gctx.fixed_delta = fixed_dt;
            update_map_gameplay(rs, input, fixed_dt);
        });

        if (rs.map_active && !rs.in_battle) {
            rs.follow_cam.update(rs.player.position(), dt);
            rs.follow_cam.apply(rs.camera);
        }

        rs.scenes.draw_overlay(rs.gctx);

        // HUD
        rs.hud.set_party(&rs.inventory);
        rs.hud.set_quests(&rs.quests);
        rs.hud.set_battle(rs.in_battle ? &rs.battle : nullptr);
        const auto hud_state = rs.hud.build(rs.gctx);
        if (frames % 60 == 0 && !hud_state.toast.empty()) {
            core::log_debug("UI", hud_state.toast);
        }

        if (renderer) {
            renderer->set_clear_color(rs.weather.clear_color_mod(base_clear));
            std::vector<render::Renderable> items;
            if (rs.map_active && rs.scene && !rs.in_battle) {
                rs.scene->collect_renderables(items);
            }
            renderer->begin_frame();
            if (rs.map_active && !rs.in_battle) {
                renderer->draw(rs.camera, items);
            }
            // ImGui HUD if available (editor links imgui; runtime may not)
            (void)game::HudBuilder::draw_imgui(hud_state);
            renderer->end_frame();
        }
        if (window) {
            window->swap_buffers();
        }

        input.end_frame();
        ++frames;

        if (options.max_frames > 0 && frames >= options.max_frames) {
            running = false;
        }
        if (options.headless && options.max_frames < 0 && frames > 3) {
            running = false;
        }
    }

    core::log_info("Runtime", "Exiting after " + std::to_string(frames) + " frame(s)");

    if (window) {
        window.reset();
        window::WindowSystem::terminate();
    }
    ctx->shutdown();
    return 0;
}

} // namespace aether::runtime
