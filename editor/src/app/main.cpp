/**
 * @file main.cpp
 * @brief AetherEditor Einstieg.
 */
#include "ui/editor_app.hpp"

#include <iostream>
#include <string>

namespace {

aether::editor::EditorAppConfig parse(int argc, char** argv) {
    aether::editor::EditorAppConfig cfg;
    for (int i = 1; i < argc; ++i) {
        const std::string s = argv[i];
        if ((s == "-p" || s == "--project") && i + 1 < argc) {
            cfg.project_path = argv[++i];
        } else if (s == "--new" && i + 1 < argc) {
            cfg.create_project = argv[++i];
        } else if (s == "--testplay") {
            cfg.auto_testplay = true;
        } else if (s == "--headless") {
            cfg.headless = true;
            cfg.force_null_window = true;
        } else if (s == "--gui") {
            cfg.headless = false;
            cfg.force_null_window = false;
        } else if ((s == "--max-frames") && i + 1 < argc) {
            cfg.max_frames = std::stoi(argv[++i]);
        } else if (s == "-h" || s == "--help") {
            std::cout
                << "AetherRPG Editor\n"
                << "  -p, --project <path>  Projekt öffnen\n"
                << "  --new <path>          Projekt anlegen\n"
                << "  --testplay            Testspiel nach dem Öffnen\n"
                << "  --gui                 Fenster + ImGui (wenn GL verfügbar)\n"
                << "  --headless            Ohne Fenster\n"
                << "  --max-frames <n>      Begrenzt Frames (CI)\n"
                << "Tabs: Projekt | Karte | Datenbank | Events | Skripte | Testspiel | Export\n";
            cfg.max_frames = 0;
        }
    }
    // Default: try GUI unless headless explicitly requested
    if (!cfg.headless && cfg.max_frames != 0) {
        // keep defaults
    }
    return cfg;
}

} // namespace

int main(int argc, char** argv) {
    try {
        auto cfg = parse(argc, argv);
        if (cfg.max_frames == 0 && cfg.project_path.empty() && cfg.create_project.empty() &&
            !cfg.auto_testplay) {
            // help only
            return 0;
        }
        // CI-friendly default when no display
        if (cfg.headless && cfg.max_frames < 0) {
            cfg.max_frames = 3;
        }
        aether::editor::EditorApp app(std::move(cfg));
        return app.run();
    } catch (const std::exception& ex) {
        std::cerr << "Fatal: " << ex.what() << '\n';
        return 2;
    }
}
