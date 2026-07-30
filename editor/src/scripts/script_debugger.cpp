/**
 * @file script_debugger.cpp
 */
#include "script_debugger.hpp"

#include <fstream>
#include <sstream>

namespace aether::editor {

void ScriptDebugger::clear_breakpoints() {
    bps_.clear();
}

void ScriptDebugger::add_breakpoint(int line) {
    if (line > 0) {
        bps_.insert(line);
    }
}

void ScriptDebugger::remove_breakpoint(int line) {
    bps_.erase(line);
}

bool ScriptDebugger::has_breakpoint(int line) const {
    return bps_.count(line) > 0;
}

ruby::EvalResult ScriptDebugger::run_file(ruby::RubyVM& vm, const std::string& path) {
    file_ = path;
    lines_.clear();
    log_.clear();
    paused_ = false;
    current_line_ = 0;

    std::ifstream in(path);
    if (!in) {
        ruby::EvalResult r;
        r.ok = false;
        r.error = "Cannot open " + path;
        return r;
    }
    std::string line;
    while (std::getline(in, line)) {
        lines_.push_back(line);
    }
    if (!enabled_ || bps_.empty()) {
        return vm.load_file(path);
    }
    return run_from(vm, 0, true, false);
}

ruby::EvalResult ScriptDebugger::step(ruby::RubyVM& vm) {
    if (!paused_) {
        ruby::EvalResult r;
        r.ok = false;
        r.error = "Not paused";
        return r;
    }
    return run_from(vm, current_line_, false, true);
}

ruby::EvalResult ScriptDebugger::cont(ruby::RubyVM& vm) {
    if (!paused_) {
        ruby::EvalResult r;
        r.ok = false;
        r.error = "Not paused";
        return r;
    }
    return run_from(vm, current_line_, true, false);
}

ruby::EvalResult ScriptDebugger::run_from(ruby::RubyVM& vm, int start, bool stop_next_bp,
                                          bool single_step) {
    ruby::EvalResult last;
    last.ok = true;
    last.value = "nil";
    paused_ = false;

    for (int i = start; i < static_cast<int>(lines_.size()); ++i) {
        const int line_no = i + 1;
        current_line_ = line_no;
        if (stop_next_bp && has_breakpoint(line_no) && !(single_step && i == start)) {
            paused_ = true;
            log_.push_back("Breakpoint @" + std::to_string(line_no) + ": " +
                           lines_[static_cast<std::size_t>(i)]);
            last.value = "break";
            return last;
        }
        log_.push_back("run L" + std::to_string(line_no));
        last = vm.eval(lines_[static_cast<std::size_t>(i)],
                       file_ + ":" + std::to_string(line_no));
        if (!last.ok) {
            paused_ = true;
            return last;
        }
        if (single_step) {
            paused_ = true;
            current_line_ = line_no + 1;
            last.value = "step";
            return last;
        }
    }
    current_line_ = static_cast<int>(lines_.size());
    paused_ = false;
    last.value = "done";
    return last;
}

} // namespace aether::editor
