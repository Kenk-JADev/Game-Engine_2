/**
 * @file bootstrap.cpp
 * @brief Game-Runtime: Projekt → Map → Player → Events → Loop.
 */
#include <aether/runtime/bootstrap.hpp>

#include <aether/aether.hpp>
#include <aether/anim/animation.hpp>
#include <aether/game/map_loader.hpp>
#include <aether/game/player.hpp>
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

void bind_ruby_game_api(ruby::RubyVM& vm, audio::AudioEngine& audio,
                        game::WeatherSystem& weather, game::GameState& state,
                        game::PlayerController& player) {
    audio::AudioEngine* ap = &audio;
    game::WeatherSystem* wp = &weather;
    game::GameState* sp = &state;
    game::PlayerController* pp = &player;

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
        {"Audio", "bgm_stop", -1, [ap](const std::vector<std::string>& args) {
             const i32 fade = args.empty() ? 0 : std::stoi(args[0]);
             ap->bgm_stop(fade);
             return std::string("nil");
         }});

    vm.define_function(
        {"Weather", "set", -1, [wp](const std::vector<std::string>& args) {
             const auto type =
                 args.empty() ? game::WeatherType::None : game::WeatherSystem::from_string(args[0]);
             const f32 power = args.size() > 1 ? std::stof(args[1]) : 5.0f;
             wp->set(type, power, 0.5f);
             return std::string("nil");
         }});
    vm.define_function({"Weather", "clear", -1, [wp](const std::vector<std::string>&) {
                            wp->clear(0.5f);
                            return std::string("nil");
                        }});

    vm.define_function(
        {"Player", "x", 0, [pp](const std::vector<std::string>&) {
             return std::to_string(pp->position().x);
         }});
    vm.define_function(
        {"Player", "y", 0, [pp](const std::vector<std::string>&) {
             return std::to_string(pp->position().y);
         }});
    vm.define_function(
        {"Player", "z", 0, [pp](const std::vector<std::string>&) {
             return std::to_string(pp->position().z);
         }});
    vm.define_function(
        {"Player", "transfer", -1, [pp](const std::vector<std::string>& args) {
             // map_id, x, y, z ignored map switch here – set pos
             if (args.size() >= 4) {
                 pp->set_position({std::stof(args[1]), std::stof(args[2]), std::stof(args[3])});
             } else if (args.size() >= 3) {
                 pp->set_position({std::stof(args[1]), 0.0f, std::stof(args[2])});
             }
             return std::string("nil");
         }});

    // Switches/variables helpers for scripts
    vm.define_function(
        {"Game", "switch", 1, [sp](const std::vector<std::string>& args) -> std::string {
             if (args.empty()) return "false";
             return sp->get_switch(static_cast<u32>(std::stoul(args[0]))) ? "true" : "false";
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

    shared::ProjectDescriptor project;
    std::string err;
    if (!shared::load_project_descriptor(options.project_path, project, &err)) {
        std::cerr << "Failed to load project: " << err << '\n';
        return 1;
    }

    core::log_info("Runtime", "Loading project '" + project.name + "' from " +
                                  project.root_dir.string());

    auto ctx = core::EngineContext::create(make_engine_config(project, options));
    ctx->start();

    res::ResourceManager resources(&ctx->thread_pool());
    resources.mount("data", project.root_dir / project.data_path);
    resources.mount("maps", project.root_dir / project.maps_path);
    resources.mount("graphics", project.root_dir / project.graphics_path);
    resources.mount("audio", project.root_dir / project.audio_path);
    resources.mount("scripts", project.root_dir / "scripts");

    // Database
    auto db_res = game::Database::load_from_directory(project.root_dir / project.data_path);
    game::Database database = db_res ? std::move(db_res.value()) : game::Database::make_default();

    window::WindowDesc wdesc = window::Window::desc_from_graphics(ctx->config().graphics);
    auto window = window::Window::create(
        wdesc, &ctx->events(),
        options.headless ? std::optional(window::WindowBackend::Null) : std::nullopt);

    render::RendererDesc rdesc;
    // Auto GL if window supports it
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

    // Map
    const auto map_file =
        game::map_path_for_id(project.root_dir / project.maps_path,
                              static_cast<u32>(project.start.map_id));
    auto map = game::load_map(map_file, &resources, renderer.get());
    if (!map.scene) {
        std::cerr << "Failed to load map\n";
        return 1;
    }
    std::unique_ptr<scene::Scene> scene = std::move(map.scene);

    // Player
    EntityId player_id = find_player(*scene);
    if (player_id == kInvalidEntity) {
        // create player
        render::Transform pt;
        pt.position = {static_cast<f32>(project.start.x), 0.0f,
                       static_cast<f32>(project.start.z)};
        pt.scale = {0.6f, 1.2f, 0.6f};
        auto mesh = render::Mesh::create_cube(1.0f);
        if (renderer) renderer->upload_mesh(*mesh);
        player_id = scene->place(scene::ObjectType::Character, "Player", mesh, pt);
        if (auto* p = scene->find(player_id)) {
            p->material.albedo = render::Color{0.2f, 0.55f, 1.0f, 1.0f};
        }
    } else {
        if (auto* p = scene->find(player_id)) {
            p->transform.position = {static_cast<f32>(project.start.x), 0.0f,
                                     static_cast<f32>(project.start.z)};
            if (p->collision_id) {
                scene->collision().set_transform(p->collision_id, p->transform);
            }
        }
    }

    game::PlayerController player;
    u32 player_col = 0;
    if (auto* p = scene->find(player_id)) {
        player_col = p->collision_id;
    }
    player.bind(scene.get(), player_id, player_col);
    player.set_position({static_cast<f32>(project.start.x), 0.0f,
                         static_cast<f32>(project.start.z)});

    game::FollowCamera follow_cam;
    follow_cam.snap(player.position());

    render::Camera camera;
    const f32 aspect = static_cast<f32>(wdesc.width) /
                       static_cast<f32>(wdesc.height > 0 ? wdesc.height : 1);
    camera.set_perspective(45.0f, aspect, 0.1f, 500.0f);
    follow_cam.apply(camera);

    // Game state + event interpreter
    game::GameState game_state;
    game::EventInterpreter interpreter(&game_state);
    std::vector<std::string> dialogue_log;

    game::WeatherSystem weather;
    anim::Animator bob_anim;
    // idle bob applied as offset on NPCs optionally – skip heavy

    // Plugins + Ruby
    plugin::PluginLoader plugins;
    const auto plug_count = plugins.scan(project.root_dir / "plugins");

    std::unique_ptr<ruby::RubyVM> vm;
    if (options.enable_ruby) {
        vm = ruby::RubyVM::create();
        vm->define_engine_api();
        bind_ruby_game_api(*vm, *audio, weather, game_state, player);

        interpreter.set_script_handler([&](const std::string& code) {
            if (vm) {
                auto r = vm->eval(code);
                if (!r.ok) {
                    core::log_warn("Event", "script: " + r.error);
                }
            }
        });
        interpreter.set_transfer_handler([&](u32 /*map_id*/, f32 x, f32 y, f32 z, i32 /*dir*/) {
            player.set_position({x, y, z});
            follow_cam.snap(player.position());
        });

        const auto entry = project.root_dir / project.scripts.entry;
        auto result = vm->load_file(entry.string());
        if (!result.ok) {
            core::log_warn("Runtime", "Script entry failed: " + result.error);
        } else {
            core::log_info("Runtime", "Scripts loaded: " + project.scripts.entry);
        }
        if (plug_count > 0) {
            auto pr = plugins.activate_all(*vm);
            if (!pr) {
                core::log_warn("Runtime", "Plugin activate: " + pr.error().what());
            }
        }
    } else {
        interpreter.set_transfer_handler([&](u32, f32 x, f32 y, f32 z, i32) {
            player.set_position({x, y, z});
            follow_cam.snap(player.position());
        });
    }

    // Viewport resize
    if (window) {
        ctx->events().subscribe<window::WindowFramebufferResizeEvent>(
            [&](const window::WindowFramebufferResizeEvent& e) {
                if (renderer && e.width > 0 && e.height > 0) {
                    renderer->set_viewport(0, 0, e.width, e.height);
                    camera.set_perspective(45.0f, static_cast<f32>(e.width) /
                                                      static_cast<f32>(e.height),
                                           0.1f, 500.0f);
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
        audio->update(dt);
        weather.update(dt);

        // Escape / cancel exits in runtime
        if (input.was_pressed("cancel") && !interpreter.is_running()) {
            // only exit if no dialogue – second press cancels
            if (options.headless) {
                // ignore
            } else {
                running = false;
            }
        }

        ctx->time().drain_fixed_steps([&](f64 fixed_dt) {
            if (interpreter.is_running()) {
                interpreter.update();
                return;
            }

            player.update_movement(input, fixed_dt, &scene->collision());

            // Interact
            if (input.was_pressed("confirm")) {
                const EntityId target = player.find_interact_target(*scene);
                if (target != kInvalidEntity) {
                    if (auto* obj = scene->find(target); obj && obj->map_event &&
                                                         !obj->map_event->pages.empty()) {
                        dialogue_log.clear();
                        interpreter.set_script_handler([&](const std::string& code) {
                            if (vm) {
                                auto r = vm->eval(code);
                                if (!r.ok) core::log_warn("Event", r.error);
                            }
                        });
                        interpreter.start(obj->map_event->pages[0].commands);
                        // drain messages into log as they appear each update
                    }
                }
            }

            if (interpreter.is_running()) {
                interpreter.update();
            }
        });

        // Collect new messages
        for (const auto& m : interpreter.messages()) {
            if (dialogue_log.empty() || dialogue_log.back() != m) {
                dialogue_log.push_back(m);
                core::log_info("Dialogue", m);
            }
        }

        follow_cam.update(player.position(), dt);
        follow_cam.apply(camera);

        if (renderer) {
            renderer->set_clear_color(weather.clear_color_mod(base_clear));
            std::vector<render::Renderable> items;
            scene->collect_renderables(items);
            renderer->begin_frame();
            renderer->draw(camera, items);
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
    (void)database;
    (void)bob_anim;

    if (window) {
        window.reset();
        window::WindowSystem::terminate();
    }
    ctx->shutdown();
    return 0;
}

} // namespace aether::runtime
