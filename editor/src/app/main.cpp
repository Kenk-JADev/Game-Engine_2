/**
 * @file main.cpp
 * @brief AetherEditor Einstieg – klassisches RPG-Maker-Bedienkonzept.
 *
 * Tabs/Bereiche (UI folgt): Projekt · Karte · Datenbank · Events ·
 * Skripte · Testspiel · Export
 *
 * Phase 1: Headless-fähige CLI + Projekt laden/anlegen, Testspiel starten.
 */
#include <aether/aether.hpp>
#include <aether/shared/project_descriptor.hpp>

#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

namespace {

void print_help() {
    std::cout
        << "AetherRPG Editor " << aether::version() << "\n"
        << "Usage: AetherEditor [options]\n"
        << "  -p, --project <path>   Open project\n"
        << "  --new <path>           Create project from template\n"
        << "  --testplay             Launch test play (headless smoke)\n"
        << "  --headless             No GUI (tools/CI)\n"
        << "  -h, --help             Help\n"
        << "\n"
        << "Editor areas (no Unity-style components):\n"
        << "  Project | Map | Database | Events | Scripts | Test Play | Export\n";
}

struct EditorArgs {
    fs::path project;
    fs::path create_new;
    bool testplay = false;
    bool headless = true; // Phase 1 default headless until GUI lands
    bool help = false;
};

EditorArgs parse(int argc, char** argv) {
    EditorArgs a;
    for (int i = 1; i < argc; ++i) {
        const std::string s = argv[i];
        if ((s == "-p" || s == "--project") && i + 1 < argc) {
            a.project = argv[++i];
        } else if (s == "--new" && i + 1 < argc) {
            a.create_new = argv[++i];
        } else if (s == "--testplay") {
            a.testplay = true;
        } else if (s == "--headless") {
            a.headless = true;
        } else if (s == "--gui") {
            a.headless = false;
        } else if (s == "-h" || s == "--help") {
            a.help = true;
        }
    }
    return a;
}

bool create_project_from_template(const fs::path& dest, std::string* err) {
    const fs::path template_root = fs::path("templates") / "empty_project";
    std::error_code ec;
    if (!fs::exists(template_root, ec)) {
        // Fallback: minimale Struktur
        fs::create_directories(dest / "data", ec);
        fs::create_directories(dest / "maps", ec);
        fs::create_directories(dest / "graphics", ec);
        fs::create_directories(dest / "audio" / "bgm", ec);
        fs::create_directories(dest / "scripts", ec);
        aether::shared::ProjectDescriptor d;
        d.name = dest.filename().string();
        d.graphics.title = d.name;
        d.root_dir = dest;
        return aether::shared::save_project_descriptor(dest / "project.json", d, err);
    }
    fs::create_directories(dest, ec);
    fs::copy(template_root, dest, fs::copy_options::recursive, ec);
    if (ec) {
        if (err) *err = ec.message();
        return false;
    }
    // Namen anpassen
    aether::shared::ProjectDescriptor d;
    if (aether::shared::load_project_descriptor(dest, d, err)) {
        d.name = dest.filename().string();
        d.graphics.title = d.name;
        return aether::shared::save_project_descriptor(dest / "project.json", d, err);
    }
    return false;
}

} // namespace

int main(int argc, char** argv) {
    const EditorArgs args = parse(argc, argv);
    if (args.help) {
        print_help();
        return 0;
    }

    aether::core::EngineConfig cfg;
    cfg.mode = aether::core::AppMode::Editor;
    cfg.log.console = true;
    cfg.log.file = false;
    cfg.log.level = "info";
    if (args.headless) {
        // Editor-Tools ohne Fenster
    }

    auto ctx = aether::core::EngineContext::create(std::move(cfg));
    ctx->start();
    AETHER_LOG_INFO("Editor", std::string("AetherEditor ") + aether::version());

    fs::path project_path = args.project;

    if (!args.create_new.empty()) {
        std::string err;
        if (!create_project_from_template(args.create_new, &err)) {
            std::cerr << "Create project failed: " << err << '\n';
            ctx->shutdown();
            return 1;
        }
        project_path = args.create_new;
        AETHER_LOG_INFO("Editor", "Created project at " + project_path.string());
    }

    if (!project_path.empty()) {
        aether::shared::ProjectDescriptor project;
        std::string err;
        if (!aether::shared::load_project_descriptor(project_path, project, &err)) {
            std::cerr << "Open project failed: " << err << '\n';
            ctx->shutdown();
            return 1;
        }
        AETHER_LOG_INFO("Editor", "Opened '" + project.name + "'");

        if (args.testplay) {
            AETHER_LOG_INFO("Editor", "Starting test play (headless smoke)…");
            // Minimaler Testplay: Ruby boot + 3 Frames über Runtime-Logik wäre ideal;
            // hier kurzer Smoke über EngineContext.
            auto vm = aether::ruby::RubyVM::create();
            vm->define_engine_api();
            const auto entry = project.root_dir / project.scripts.entry;
            auto r = vm->load_file(entry.string());
            if (!r.ok) {
                AETHER_LOG_WARN("Editor", "Testplay script: " + r.error);
            }
            for (int i = 0; i < 3; ++i) {
                ctx->pump_frame();
            }
            AETHER_LOG_INFO("Editor", "Test play finished");
        }
    } else {
        AETHER_LOG_INFO("Editor", "No project opened. Use --project or --new.");
        print_help();
    }

    ctx->shutdown();
    return 0;
}
