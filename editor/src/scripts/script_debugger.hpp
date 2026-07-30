/**
 * @file script_debugger.hpp
 * @brief Minimaler Script-Debugger: Breakpoints + Step über Stub/MRuby eval wrapper.
 *
 * Kein voller Bytecode-Debugger – speichert Breakpoint-Zeilen und kann
 * skriptzeilenweise ausführen (Stub) bzw. vor eval prüfen.
 */
#pragma once

#include <aether/ruby/ruby_vm.hpp>

#include <set>
#include <string>
#include <vector>

namespace aether::editor {

struct DebugFrame {
    std::string file;
    int line = 0;
    std::string source_line;
};

class ScriptDebugger {
public:
    void clear_breakpoints();
    void add_breakpoint(int line);
    void remove_breakpoint(int line);
    [[nodiscard]] bool has_breakpoint(int line) const;
    [[nodiscard]] const std::set<int>& breakpoints() const noexcept { return bps_; }

    void set_enabled(bool v) noexcept { enabled_ = v; }
    [[nodiscard]] bool enabled() const noexcept { return enabled_; }

    /**
     * @brief Führt Datei zeilenweise aus; stoppt an Breakpoints (nur Stub-tauglich).
     * Bei Stop: paused_ = true, current_line gesetzt.
     */
    ruby::EvalResult run_file(ruby::RubyVM& vm, const std::string& path);

    /**
     * @brief Nächste Zeile wenn pausiert.
     */
    ruby::EvalResult step(ruby::RubyVM& vm);

    /**
     * @brief Weiter bis nächster Breakpoint / Ende.
     */
    ruby::EvalResult cont(ruby::RubyVM& vm);

    [[nodiscard]] bool paused() const noexcept { return paused_; }
    [[nodiscard]] int current_line() const noexcept { return current_line_; }
    [[nodiscard]] const std::vector<std::string>& log() const noexcept { return log_; }

private:
    ruby::EvalResult run_from(ruby::RubyVM& vm, int start, bool stop_next_bp, bool single_step);

    std::set<int> bps_;
    bool enabled_ = false;
    bool paused_ = false;
    int current_line_ = 0;
    std::string file_;
    std::vector<std::string> lines_;
    std::vector<std::string> log_;
};

} // namespace aether::editor
