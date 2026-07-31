/**
 * @file testplay_runner.cpp
 */
#include "testplay_runner.hpp"

#include <aether/core/logger.hpp>

#include <cstdlib>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace aether::editor {
namespace fs = std::filesystem;

std::string shell_quote(const fs::path& p) {
    std::string s = p.string();
    std::string out;
    out.reserve(s.size() + 2);
    out.push_back('"');
    for (const char c : s) {
        if (c == '"' || c == '\\') {
            out.push_back('\\');
        }
        out.push_back(c);
    }
    out.push_back('"');
    return out;
}

fs::path executable_dir() {
#if defined(_WIN32)
    wchar_t buf[MAX_PATH + 2] = {};
    const DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH + 1);
    if (n > 0) {
        fs::path p(buf, buf + n);
        return p.parent_path();
    }
    std::error_code ec;
    return fs::current_path(ec);
#else
    // Linux/macOS: /proc/self/exe (bzw. _NSGetExecutablePath auf macOS wäre
    // nötig; /proc existiert auf Linux, auf macOS greift der CWD-Fallback)
    std::error_code ec;
    fs::path exe = fs::read_symlink("/proc/self/exe", ec);
    if (!ec) {
        return exe.parent_path();
    }
    return fs::current_path(ec);
#endif
}

fs::path find_game_binary() {
    std::error_code ec;
    const fs::path dirs[] = {executable_dir(), fs::current_path(ec)};
#if defined(_WIN32)
    const char* names[] = {"Game.exe", "Game"};
#else
    const char* names[] = {"Game", "Game.exe"};
#endif
    for (const auto& dir : dirs) {
        for (const char* n : names) {
            const fs::path cand = dir / n;
            if (fs::exists(cand, ec) && fs::is_regular_file(cand, ec)) {
                return cand;
            }
        }
    }
    return {};
}

std::string build_testplay_command(const fs::path& game_binary,
                                   const fs::path& project_dir) {
#if defined(_WIN32)
    return std::string("start \"AetherRPG Testspiel\" ") + shell_quote(game_binary) +
           " --project " + shell_quote(project_dir);
#else
    return shell_quote(game_binary) + " --project " + shell_quote(project_dir) +
           " >/dev/null 2>&1 &";
#endif
}

bool start_testplay(const fs::path& project_dir, std::string& out_error) {
    const fs::path game = find_game_binary();
    if (game.empty()) {
        out_error =
            "Game-Runtime nicht gefunden. Bitte erst bauen (cmake --build build "
            "--target Game) – Game muss neben AetherEditor liegen.";
        core::log_error("TestPlay", out_error);
        return false;
    }
    const std::string cmd = build_testplay_command(game, project_dir);
    const int rc = std::system(cmd.c_str());
    if (rc != 0) {
        out_error = "Testspiel-Start fehlgeschlagen (Shell-Rückgabe " +
                    std::to_string(rc) + ")";
        core::log_error("TestPlay", out_error);
        return false;
    }
    core::log_info("TestPlay", "gestartet: " + cmd);
    return true;
}

} // namespace aether::editor
