/**
 * @file testplay_runner.hpp
 * @brief Startet die Game-Runtime als „Testspiel“ (eigener Prozess).
 *
 * Klassischer RPG-Maker-Workflow: „Testspiel“ öffnet das Projekt in der
 * echten Runtime (Game / Game.exe) mit einem eigenen Fenster – nicht als
 * Editor-Smoke. Die Kommandzeile ist testbar (build_testplay_command),
 * der Prozessstart bewusst simpel (std::system + Shell-Detach).
 */
#pragma once

#include <filesystem>
#include <string>

namespace aether::editor {

/**
 * @brief Quoted Pfad für Shell-Aufrufe (Leerzeichen, Anführungszeichen).
 */
[[nodiscard]] std::string shell_quote(const std::filesystem::path& p);

/**
 * @brief Verzeichnis der aktuell laufenden Executable (robust, ohne argv).
 */
[[nodiscard]] std::filesystem::path executable_dir();

/**
 * @brief Sucht die Game-Runtime (Game / Game.exe) neben der Editor-Executable
 *        oder im aktuellen Arbeitsverzeichnis.
 * @return leerer Pfad wenn nicht gefunden
 */
[[nodiscard]] std::filesystem::path find_game_binary();

/**
 * @brief Baut die Kommandozeile fürs Testspiel (testbar ohne Prozessstart).
 *
 * Windows: `start "Titel" "Game.exe" --project "dir"`
 * POSIX:   `"Game" --project "dir" >/dev/null 2>&1 &` (detached)
 */
[[nodiscard]] std::string build_testplay_command(
    const std::filesystem::path& game_binary,
    const std::filesystem::path& project_dir);

/**
 * @brief Startet das Testspiel als eigenen, nicht blockierenden Prozess.
 * @return true wenn der Start ausgelöst wurde; sonst Fehlertext in out_error.
 */
bool start_testplay(const std::filesystem::path& project_dir,
                    std::string& out_error);

} // namespace aether::editor
