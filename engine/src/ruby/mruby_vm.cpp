/**
 * @file mruby_vm.cpp
 * @brief mruby backend for Aether Ruby API.
 */
#include "mruby_vm.hpp"

#if defined(AETHER_WITH_MRUBY)

#include <aether/core/logger.hpp>

#include <mruby.h>
#include <mruby/array.h>
#include <mruby/class.h>
#include <mruby/compile.h>
#include <mruby/error.h>
#include <mruby/string.h>
#include <mruby/variable.h>

#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace aether::ruby {
namespace {

MRubyVM* g_current_vm = nullptr;

std::string mrb_value_to_string(mrb_state* mrb, mrb_value v) {
    if (mrb_nil_p(v)) {
        return "nil";
    }
    if (mrb_true_p(v)) {
        return "true";
    }
    if (mrb_false_p(v)) {
        return "false";
    }
    if (mrb_fixnum_p(v)) {
        return std::to_string(static_cast<long long>(mrb_fixnum(v)));
    }
    if (mrb_symbol_p(v)) {
        const char* n = mrb_sym_name(mrb, mrb_symbol(v));
        return n ? std::string(n) : std::string();
    }
    mrb_value s = mrb_funcall(mrb, v, "to_s", 0);
    if (mrb_string_p(s)) {
        return std::string(RSTRING_PTR(s), static_cast<std::size_t>(RSTRING_LEN(s)));
    }
    return "nil";
}

std::string class_basename(mrb_state* mrb, mrb_value self) {
    // Module functions receive the module as self
    mrb_value name_v = mrb_funcall(mrb, self, "name", 0);
    if (mrb_string_p(name_v)) {
        std::string full(RSTRING_PTR(name_v), static_cast<std::size_t>(RSTRING_LEN(name_v)));
        const auto pos = full.rfind("::");
        if (pos != std::string::npos) {
            return full.substr(pos + 2);
        }
        return full;
    }
    return {};
}

mrb_value generic_module_method(mrb_state* mrb, mrb_value self) {
    if (!g_current_vm) {
        return mrb_nil_value();
    }

    mrb_callinfo* ci = mrb->c->ci;
    if (!ci) {
        return mrb_nil_value();
    }

    const char* method = mrb_sym_name(mrb, ci->mid);
    if (!method) {
        return mrb_nil_value();
    }

    std::string mod = class_basename(mrb, self);

    mrb_value* argv = nullptr;
    mrb_int argc = 0;
    mrb_get_args(mrb, "*", &argv, &argc);

    std::vector<std::string> args;
    args.reserve(static_cast<std::size_t>(argc));
    for (mrb_int i = 0; i < argc; ++i) {
        args.push_back(mrb_value_to_string(mrb, argv[i]));
    }

    auto result = g_current_vm->dispatch_host(mod, method, args);
    if (!result.ok) {
        mrb_raisef(mrb, E_RUNTIME_ERROR, "%s", result.error.c_str());
    }
    if (result.value.empty() || result.value == "nil") {
        return mrb_nil_value();
    }
    if (result.value == "true") {
        return mrb_true_value();
    }
    if (result.value == "false") {
        return mrb_false_value();
    }
    try {
        std::size_t idx = 0;
        const long v = std::stol(result.value, &idx, 10);
        if (idx == result.value.size()) {
            return mrb_fixnum_value(static_cast<mrb_int>(v));
        }
    } catch (...) {
    }
    return mrb_str_new_cstr(mrb, result.value.c_str());
}

} // namespace

MRubyVM::MRubyVM() : RubyVM(RubyBackend::MRuby) {
    mrb_ = mrb_open();
    if (!mrb_) {
        core::log_error("Ruby", "mrb_open failed");
        return;
    }
    g_current_vm = this;
    core::log_info("Ruby", "MRubyVM created");
}

MRubyVM::~MRubyVM() {
    if (g_current_vm == this) {
        g_current_vm = nullptr;
    }
    if (mrb_) {
        mrb_close(mrb_);
        mrb_ = nullptr;
    }
}

void MRubyVM::define_engine_api() {
    if (!mrb_) {
        return;
    }

    auto noop = [](const std::vector<std::string>&) -> std::string { return "nil"; };

    define_function({"Graphics", "frame_rate", 0,
                     [](const std::vector<std::string>&) { return "60"; }});
    define_function({"Graphics", "frame_rate=", 1, noop});
    define_function({"Graphics", "width", 0,
                     [](const std::vector<std::string>&) { return "1280"; }});
    define_function({"Graphics", "height", 0,
                     [](const std::vector<std::string>&) { return "720"; }});
    define_function({"Graphics", "update", 0, noop});

    define_function({"Audio", "bgm_play", -1, noop});
    define_function({"Audio", "bgm_stop", -1, noop});
    define_function({"Audio", "bgs_play", -1, noop});
    define_function({"Audio", "bgs_stop", -1, noop});
    define_function({"Audio", "me_play", -1, noop});
    define_function({"Audio", "me_stop", -1, noop});
    define_function({"Audio", "se_play", -1, noop});
    define_function({"Audio", "se_stop", 0, noop});

    define_function({"Input", "press?", 1,
                     [](const std::vector<std::string>&) { return "false"; }});
    define_function({"Input", "trigger?", 1,
                     [](const std::vector<std::string>&) { return "false"; }});
    define_function({"Input", "repeat?", 1,
                     [](const std::vector<std::string>&) { return "false"; }});

    define_function({"SceneManager", "goto", 1, noop});
    define_function({"SceneManager", "call", 1, noop});
    define_function({"SceneManager", "return", 0, noop});

    define_function({"Player", "transfer", -1, noop});
    define_function({"Player", "x", 0, [](const std::vector<std::string>&) { return "0"; }});
    define_function({"Player", "y", 0, [](const std::vector<std::string>&) { return "0"; }});
    define_function({"Player", "z", 0, [](const std::vector<std::string>&) { return "0"; }});

    define_function({"NPC", "find", 1, [](const std::vector<std::string>& args) {
                         return args.empty() ? "nil" : ("NPC(" + args[0] + ")");
                     }});
    define_function({"Enemy", "spawn", -1, noop});
    define_function({"Camera", "move_to", -1, noop});
    define_function({"Camera", "shake", -1, noop});
    define_function({"Weather", "set", -1, noop});
    define_function({"Weather", "clear", -1, noop});
    define_function({"Inventory", "gain", -1, noop});
    define_function({"Inventory", "lose", -1, noop});
    define_function({"Inventory", "count", 1,
                     [](const std::vector<std::string>&) { return "0"; }});
    define_function({"Quest", "start", 1, noop});
    define_function({"Quest", "complete", 1, noop});
    define_function({"Quest", "active?", 1,
                     [](const std::vector<std::string>&) { return "false"; }});
    define_function({"Dialogue", "start", 1, noop});
    define_function({"Dialogue", "choice", -1,
                     [](const std::vector<std::string>&) { return "0"; }});
    define_function({"Map", "id", 0, [](const std::vector<std::string>&) { return "0"; }});
    define_function({"Map", "tint", -1, noop});
    define_function({"Map", "scroll", -1, noop});

    register_host_as_ruby();
    core::log_info("Ruby", "Engine API modules defined (mruby)");
}

void MRubyVM::register_host_as_ruby() {
    if (!mrb_) {
        return;
    }

    std::unordered_map<std::string, struct RClass*> mods;
    for (const auto& h : host_functions_) {
        struct RClass* mod = nullptr;
        const auto it = mods.find(h.module);
        if (it == mods.end()) {
            mod = mrb_define_module(mrb_, h.module.c_str());
            mods.emplace(h.module, mod);
        } else {
            mod = it->second;
        }

        mrb_aspec aspec = MRB_ARGS_ANY();
        if (h.arity == 0) {
            aspec = MRB_ARGS_NONE();
        } else if (h.arity > 0) {
            aspec = MRB_ARGS_REQ(h.arity);
        }

        mrb_define_module_function(mrb_, mod, h.name.c_str(), generic_module_method, aspec);
    }
}

EvalResult MRubyVM::dispatch_host(std::string_view module, std::string_view name,
                                  const std::vector<std::string>& args) {
    return call_host(module, name, args);
}

EvalResult MRubyVM::eval(std::string_view source, std::string_view filename) {
    EvalResult r;
    if (!mrb_) {
        r.ok = false;
        r.error = "mruby not initialized";
        last_error_ = r.error;
        return r;
    }
    g_current_vm = this;

    mrbc_context* ctx = mrbc_context_new(mrb_);
    mrbc_filename(mrb_, ctx, std::string(filename).c_str());

    const mrb_value result = mrb_load_nstring_cxt(
        mrb_, source.data(), static_cast<mrb_int>(source.size()), ctx);
    mrbc_context_free(mrb_, ctx);

    if (mrb_->exc) {
        const mrb_value exc = mrb_obj_value(mrb_->exc);
        mrb_->exc = nullptr;
        r.ok = false;
        const mrb_value msg = mrb_funcall(mrb_, exc, "inspect", 0);
        r.error = mrb_value_to_string(mrb_, mrb_string_p(msg) ? msg : exc);
        last_error_ = r.error;
        return r;
    }

    r.ok = true;
    r.value = mrb_value_to_string(mrb_, result);
    return r;
}

EvalResult MRubyVM::load_file(std::string_view path) {
    const std::string path_str(path);
    std::ifstream in{path_str};
    if (!in) {
        EvalResult r;
        r.ok = false;
        r.error = "Cannot open script: " + path_str;
        last_error_ = r.error;
        return r;
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    load_history_.push_back(path_str);
    return eval(ss.str(), path_str);
}

void MRubyVM::set_global(std::string_view name, std::string_view value) {
    globals_[std::string(name)] = std::string(value);
    if (!mrb_) {
        return;
    }
    const std::string gname = std::string("$") + std::string(name);
    mrb_gv_set(mrb_, mrb_intern_cstr(mrb_, gname.c_str()),
               mrb_str_new_cstr(mrb_, std::string(value).c_str()));
}

std::string MRubyVM::get_global(std::string_view name) const {
    if (mrb_) {
        const std::string gname = std::string("$") + std::string(name);
        const mrb_value v =
            mrb_gv_get(const_cast<mrb_state*>(mrb_),
                       mrb_intern_cstr(const_cast<mrb_state*>(mrb_), gname.c_str()));
        if (!mrb_nil_p(v)) {
            return mrb_value_to_string(const_cast<mrb_state*>(mrb_), v);
        }
    }
    const auto it = globals_.find(std::string(name));
    return it == globals_.end() ? "" : it->second;
}

} // namespace aether::ruby

#endif // AETHER_WITH_MRUBY
