/**
 * @file bootstrap.cpp
 * @brief Schlanke Game-Runtime: Projekt laden → Engine booten → Loop.
 */
#include <aether/runtime/bootstrap.hpp>

#include <aether/aether.hpp>
#include <aether/shared/project_descriptor.hpp>

#include <iostream>
#include <string>

namespace aether::runtime {
namespace {

core::EngineConfig make_engine_config(const shared::ProjectDescriptor& project,
                                      const RuntimeOptions& opt) {
    core::EngineConfig cfg;
    cfg.mode = opt.headless ? core::AppMode::Headless : core::AppMode::Runtime;
    cfg.graphics.title = project.graphics.title.empty() ? project.name
                                                        : project.graphics.title;
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
            opt.max_frames = 0; // signal help-only
        } else if (!a.empty() && a[0] != '-') {
            opt.project_path = a;
        }
    }
    return opt;
}

int run_game(const RuntimeOptions& options) {
    if (options.max_frames == 0) {
        return 0; // help
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

    // Resources
    res::ResourceManager resources(&ctx->thread_pool());
    resources.mount("data", project.root_dir / project.data_path);
    resources.mount("maps", project.root_dir / project.maps_path);
    resources.mount("graphics", project.root_dir / project.graphics_path);
    resources.mount("audio", project.root_dir / project.audio_path);
    resources.mount("scripts", project.root_dir / "scripts");

    // Window
    window::WindowDesc wdesc = window::Window::desc_from_graphics(ctx->config().graphics);
    auto window = window::Window::create(
        wdesc, &ctx->events(),
        options.headless ? std::optional(window::WindowBackend::Null)
                         : std::nullopt);

    // Renderer / Input / Audio
    render::RendererDesc rdesc;
    rdesc.backend = render::RendererBackend::Null;
    auto renderer = render::Renderer::create(rdesc, window.get());

    input::InputManager input(&ctx->events());
    input.register_default_rpg_actions();
    if (window) {
        input.attach_window(window.get());
    }

    core::AudioConfig acfg = ctx->config().audio;
    auto audio = audio::AudioEngine::create(audio::AudioBackend::Null, acfg);

    // Ruby
    std::unique_ptr<ruby::RubyVM> vm;
    if (options.enable_ruby) {
        vm = ruby::RubyVM::create(ruby::RubyBackend::Stub);
        vm->define_engine_api();

        // Bind Audio host to real engine
        audio::AudioEngine* audio_ptr = audio.get();
        vm->define_function(
            {"Audio", "bgm_play", -1,
             [audio_ptr](const std::vector<std::string>& args) -> std::string {
                 if (!args.empty()) {
                     audio::PlayParams p;
                     if (args.size() > 1) p.volume = std::stoi(args[1]);
                     if (args.size() > 2) p.pitch = std::stoi(args[2]);
                     audio_ptr->bgm_play(args[0], p);
                 }
                 return "nil";
             }});

        const auto entry = project.root_dir / project.scripts.entry;
        auto result = vm->load_file(entry.string());
        if (!result.ok) {
            core::log_warn("Runtime", "Script entry failed: " + result.error);
        } else {
            core::log_info("Runtime", "Scripts loaded: " + project.scripts.entry);
        }
    }

    // Einfache Start-Kamera / Boden
    render::Camera camera;
    const f32 aspect = static_cast<f32>(wdesc.width) /
                       static_cast<f32>(wdesc.height > 0 ? wdesc.height : 1);
    camera.set_perspective(45.0f, aspect, 0.1f, 500.0f);
    camera.look_at({static_cast<f32>(project.start.x), 8.0f,
                    static_cast<f32>(project.start.z) + 12.0f},
                   {static_cast<f32>(project.start.x), 0.0f,
                    static_cast<f32>(project.start.z)},
                   {0, 1, 0});

    auto ground = render::Mesh::create_plane(40.0f);
    renderer->upload_mesh(*ground);
    std::vector<render::Renderable> scene;
    {
        render::Renderable r;
        r.mesh = ground;
        r.material.albedo = render::Color{0.35f, 0.55f, 0.30f, 1.0f};
        scene.push_back(r);
    }

    core::log_info("Runtime", "Entering main loop");

    i64 frames = 0;
    bool running = true;

    // Close-Event
    ctx->events().subscribe<window::WindowCloseEvent>(
        [&](const window::WindowCloseEvent&) { running = false; });

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

        ctx->time().drain_fixed_steps([&](f64 fixed_dt) {
            (void)fixed_dt;
            // Spiel-Simulation (Player, Events, …) – später
        });

        if (renderer) {
            renderer->begin_frame();
            renderer->draw(camera, scene);
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

        // Headless ohne Fenster: nach max_frames oder Escape-Simulation beenden
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
