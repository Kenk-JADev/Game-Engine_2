/**
 * @file export_test.cpp
 * @brief Tests für den Projekt-Export (editor::export_project).
 *
 * Erzeugt ein Mini-Projekt, exportiert es und prüft die Paket-Struktur.
 */
#include "../../editor/src/export/exporter.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

using namespace aether::editor;
namespace fs = std::filesystem;

static int g_failures = 0;
#define CHECK(cond)                                                            \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::fprintf(stderr, "CHECK failed: %s (%s:%d)\n", #cond,         \
                         __FILE__, __LINE__);                                  \
            ++g_failures;                                                      \
        }                                                                      \
    } while (0)

int main() {
    const fs::path root = fs::temp_directory_path() / "aether_export_test";
    fs::remove_all(root);
    const fs::path project = root / "MyGame";
    const fs::path out = root / "MyGame_Export";

    // --- Mini-Projekt anlegen -------------------------------------------------
    fs::create_directories(project / "data");
    fs::create_directories(project / "maps");
    fs::create_directories(project / "graphics");
    fs::create_directories(project / "audio" / "bgm");
    fs::create_directories(project / "scripts");
    fs::create_directories(project / "plugins");
    {
        std::ofstream(project / "project.json")
            << R"({"name":"MyGame","graphics":{"width":1280,"height":720}})";
        std::ofstream(project / "data" / "actors.json") << R"({"actors":[]})";
        std::ofstream(project / "maps" / "map001.json") << R"({"name":"Map"})";
        std::ofstream(project / "scripts" / "main.rb") << "# test\n";
        std::ofstream(project / "audio" / "bgm" / "Theme1.wav") << "RIFFxxxxWAVE";
    }

    // --- Fehlerpfad: Projekt fehlt --------------------------------------------
    {
        auto r = export_project(ExportOptions{root / "Nope", out / "nope", {}});
        CHECK(!r.ok);
    }

    // --- Export ohne Binary ----------------------------------------------------
    {
        ExportOptions opt;
        opt.project_dir = project;
        opt.output_dir = out;
        opt.copy_engine_assets = false;
        auto r = export_project(opt);
        CHECK(r.ok);
        CHECK(fs::exists(out / "project.json"));
        CHECK(fs::exists(out / "data" / "actors.json"));
        CHECK(fs::exists(out / "maps" / "map001.json"));
        CHECK(fs::exists(out / "scripts" / "main.rb"));
        CHECK(fs::exists(out / "audio" / "bgm" / "Theme1.wav"));
        CHECK(fs::exists(out / "README_PLAY.txt"));
        // leere Ordner werden angelegt
        CHECK(fs::is_directory(out / "graphics"));
        CHECK(fs::is_directory(out / "plugins"));
    }

    // --- Export mit Binary (Rechte + Name) --------------------------------------
    {
        const fs::path game =
            root / (std::string("Game") + (fs::exists("C:\\") ? ".exe" : ""));
        {
            std::ofstream b(game, std::ios::binary);
            b << "FAKE-BINARY";
        }
        fs::remove_all(out);
        ExportOptions opt;
        opt.project_dir = project;
        opt.output_dir = out;
        opt.game_binary = game;
        opt.copy_engine_assets = false;
        auto r = export_project(opt);
        CHECK(r.ok);
        CHECK(fs::exists(out / game.filename()));
        // Doppelter Export überschreibt sauber
        auto r2 = export_project(opt);
        CHECK(r2.ok);
        fs::remove(game);
    }

    fs::remove_all(root);

    if (g_failures == 0) {
        std::puts("OK: export_test passed");
        return 0;
    }
    std::fprintf(stderr, "FAIL: %d\n", g_failures);
    return 1;
}
