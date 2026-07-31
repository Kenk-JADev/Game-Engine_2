/**
 * @file bootstrap.hpp
 * @brief Game-Runtime Bootstrap – lädt Projekt und startet die Engine.
 */
#pragma once

#include <aether/shared/project_descriptor.hpp>

#include <filesystem>
#include <string>

namespace aether::runtime {

struct RuntimeOptions {
    std::filesystem::path project_path = "."; ///< project.json oder Ordner
    bool headless = false;                    ///< für CI / automatisierte Tests
    int max_frames = -1;                      ///< <0 = unbegrenzt; sonst Smoke-Lauf
    bool enable_ruby = true;
    std::string log_level;                    ///< leer = default (headless: warn)
};

/**
 * @brief Einstieg der Runtime.
 * @return Prozess-Exitcode (0 = ok)
 */
int run_game(const RuntimeOptions& options);

/**
 * @brief Parst argv in RuntimeOptions.
 */
RuntimeOptions parse_args(int argc, char** argv);

} // namespace aether::runtime
