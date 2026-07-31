/**
 * @file testplay_test.cpp
 * @brief Testspiel-Runner: Shell-Quoting, Kommandozeile, Binary-Suche.
 *
 * Der echte Prozessstart wird hier nicht ausgelöst (würde ein Spiel-Fenster
 * öffnen); getestet werden die deterministischen Bausteine.
 */
#include "../../editor/src/testplay/testplay_runner.hpp"

#include <cstdio>
#include <filesystem>
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
    // --- shell_quote ----------------------------------------------------------
    CHECK(shell_quote(fs::path("My Game/Projekt")) == "\"My Game/Projekt\"");
    CHECK(shell_quote(fs::path("plain")) == "\"plain\"");
    CHECK(shell_quote(fs::path("a\"b")) == "\"a\\\"b\"");

    // --- build_testplay_command -----------------------------------------------
    {
        const std::string cmd =
            build_testplay_command(fs::path("/opt/Game"), fs::path("/tmp/Mein Spiel"));
        CHECK(cmd.find("--project") != std::string::npos);
        CHECK(cmd.find("\"/tmp/Mein Spiel\"") != std::string::npos);
        CHECK(cmd.find("\"/opt/Game\"") != std::string::npos);
#if defined(_WIN32)
        CHECK(cmd.find("start \"AetherRPG Testspiel\"") == 0);
#else
        CHECK(cmd.find("&") != std::string::npos); // detached
#endif
    }

    // --- executable_dir / find_game_binary -------------------------------------
    {
        const fs::path dir = executable_dir();
        CHECK(!dir.empty());
        CHECK(fs::is_directory(dir));
        // Das Test-Binary liegt neben der Game-Runtime (build/bin) → Suche
        // muss das echte Testspiel-Binary finden (wird im Build erzeugt).
        const fs::path game = find_game_binary();
        if (fs::exists(dir / "Game") || fs::exists(dir / "Game.exe")) {
            CHECK(!game.empty());
            if (!game.empty()) {
                CHECK(fs::exists(game));
                CHECK(game.filename() == "Game" || game.filename() == "Game.exe");
            }
        } else {
            // Kein Game-Binary in der Nähe (z. B. einzelner Testlauf):
            // find_game_binary darf dann nur einen leeren Pfad liefern.
            CHECK(game.empty());
        }
    }

    if (g_failures == 0) {
        std::puts("OK: testplay_test passed");
        return 0;
    }
    std::fprintf(stderr, "FAIL: %d\n", g_failures);
    return 1;
}
