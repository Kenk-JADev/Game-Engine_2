/**
 * @file ruby_vm.cpp
 * @brief Ruby-VM Basis + Stub-Backend.
 */
#include <aether/ruby/ruby_vm.hpp>
#include <aether/core/logger.hpp>

#include <cctype>
#include <fstream>
#include <sstream>

namespace aether::ruby {
namespace {

/**
 * @brief Stub-VM: führt kein echtes Ruby aus, unterstützt aber
 * Host-Calls und speichert geladene Quellen für Hot-Reload-Tests.
 */
class StubRubyVM final : public RubyVM {
public:
    StubRubyVM() : RubyVM(RubyBackend::Stub) {
        core::log_info("Ruby", "StubRubyVM created (mruby optional later)");
    }

    void define_engine_api() override {
        // Bootstrap-Quelle „ausführen“ (nur merken)
        const std::string boot = engine_api_bootstrap_source();
        sources_["<bootstrap>"] = boot;

        // Host-Bindings als No-Op-Stubs (werden von Engine später überschrieben)
        auto noop = [](const std::vector<std::string>&) -> std::string { return "nil"; };

        define_function({"Graphics", "frame_rate=", 1, noop});
        define_function({"Graphics", "width", 0, [](const std::vector<std::string>&) {
                            return "1280";
                        }});
        define_function({"Graphics", "height", 0, [](const std::vector<std::string>&) {
                            return "720";
                        }});

        define_function({"Audio", "bgm_play", -1,
                         [this](const std::vector<std::string>& args) {
                             last_audio_call_ = "bgm_play";
                             if (!args.empty()) last_audio_arg_ = args[0];
                             return "nil";
                         }});
        define_function({"Audio", "se_play", -1,
                         [this](const std::vector<std::string>& args) {
                             last_audio_call_ = "se_play";
                             if (!args.empty()) last_audio_arg_ = args[0];
                             return "nil";
                         }});
        define_function({"Audio", "bgm_stop", -1, noop});

        define_function({"Input", "press?", 1, [](const std::vector<std::string>&) {
                            return "false";
                        }});
        define_function({"Input", "trigger?", 1, [](const std::vector<std::string>&) {
                            return "false";
                        }});

        define_function({"SceneManager", "goto", 1, noop});
        define_function({"Player", "transfer", -1, noop});
        define_function({"Map", "tint", -1, noop});
        define_function({"Weather", "set", -1, noop});
        define_function({"Inventory", "gain", -1, noop});
        define_function({"Quest", "start", 1, noop});
        define_function({"Dialogue", "start", 1, noop});
        define_function({"NPC", "find", 1, [](const std::vector<std::string>& args) {
                            return args.empty() ? "nil" : ("NPC(" + args[0] + ")");
                        }});
        define_function({"Enemy", "spawn", -1, noop});
        define_function({"Camera", "move_to", -1, noop});

        core::log_info("Ruby", "Engine API modules defined (stub)");
    }

    EvalResult eval_line(std::string src) {
        while (!src.empty() && (src.back() == '\n' || src.back() == '\r' || src.back() == ' ' ||
                                src.back() == ';'))
            src.pop_back();
        while (!src.empty() && (src.front() == ' ' || src.front() == '\t'))
            src.erase(src.begin());
        if (src.empty() || src.starts_with("#") || src.starts_with("module ") ||
            src.starts_with("class ") || src.starts_with("end") || src.starts_with("def ") ||
            src.starts_with("module_function") || src == "true" || src == "false" ||
            src == "nil") {
            EvalResult r;
            r.ok = true;
            r.value = "nil";
            return r;
        }

        if (src.starts_with("return ")) {
            EvalResult r;
            r.ok = true;
            r.value = src.substr(7);
            if (r.value.size() >= 2 && r.value.front() == '"' && r.value.back() == '"') {
                r.value = r.value.substr(1, r.value.size() - 2);
            }
            return r;
        }

        if (src.size() > 1 && src[0] == '$') {
            const auto eq = src.find('=');
            if (eq != std::string::npos) {
                std::string name = src.substr(1, eq - 1);
                while (!name.empty() && name.back() == ' ') name.pop_back();
                std::string val = src.substr(eq + 1);
                while (!val.empty() && val.front() == ' ') val.erase(val.begin());
                if (val.size() >= 2 && val.front() == '"' && val.back() == '"') {
                    val = val.substr(1, val.size() - 2);
                }
                set_global(name, val);
                EvalResult r;
                r.ok = true;
                r.value = val;
                return r;
            }
        }

        // Only treat as Module.method if Module starts with uppercase letter
        const auto dot = src.find('.');
        if (dot != std::string::npos && dot > 0 && std::isupper(static_cast<unsigned char>(src[0]))) {
            std::string mod = src.substr(0, dot);
            // module name must be identifier
            bool ok_mod = true;
            for (char c : mod) {
                if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_')) ok_mod = false;
            }
            if (ok_mod) {
                std::string rest = src.substr(dot + 1);
                std::string method;
                std::vector<std::string> args;
                const auto paren = rest.find('(');
                if (paren == std::string::npos) {
                    method = rest;
                } else {
                    method = rest.substr(0, paren);
                    std::string inside = rest.substr(paren + 1);
                    if (!inside.empty() && inside.back() == ')') inside.pop_back();
                    std::string cur;
                    for (char c : inside) {
                        if (c == ',') {
                            while (!cur.empty() && cur.front() == ' ') cur.erase(cur.begin());
                            while (!cur.empty() && cur.back() == ' ') cur.pop_back();
                            if (cur.size() >= 2 && cur.front() == '"' && cur.back() == '"')
                                cur = cur.substr(1, cur.size() - 2);
                            if (cur.starts_with(":")) cur = cur.substr(1);
                            args.push_back(cur);
                            cur.clear();
                        } else {
                            cur.push_back(c);
                        }
                    }
                    if (!cur.empty()) {
                        while (!cur.empty() && cur.front() == ' ') cur.erase(cur.begin());
                        while (!cur.empty() && cur.back() == ' ') cur.pop_back();
                        if (cur.size() >= 2 && cur.front() == '"' && cur.back() == '"')
                            cur = cur.substr(1, cur.size() - 2);
                        if (cur.starts_with(":")) cur = cur.substr(1);
                        args.push_back(cur);
                    }
                }
                // Skip unknown host methods quietly for multi-line boot scripts
                if (find_host(mod, method)) {
                    return call_host(mod, method, args);
                }
            }
        }

        EvalResult r;
        r.ok = true;
        r.value = "nil";
        return r;
    }

    EvalResult eval(std::string_view source, std::string_view filename) override {
        sources_[std::string(filename)] = std::string(source);
        EvalResult last;
        last.ok = true;
        last.value = "nil";

        std::string line;
        for (char c : source) {
            if (c == '\n') {
                auto r = eval_line(line);
                if (!r.ok) return r;
                last = r;
                line.clear();
            } else {
                line.push_back(c);
            }
        }
        if (!line.empty()) {
            auto r = eval_line(line);
            if (!r.ok) return r;
            last = r;
        }
        return last;
    }

    EvalResult load_file(std::string_view path) override {
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
        load_history_.emplace_back(path_str);
        return eval(ss.str(), path_str);
    }

    void set_global(std::string_view name, std::string_view value) override {
        globals_[std::string(name)] = std::string(value);
    }

    std::string get_global(std::string_view name) const override {
        const auto it = globals_.find(std::string(name));
        return it == globals_.end() ? "" : it->second;
    }

    std::string last_audio_call_;
    std::string last_audio_arg_;
    std::unordered_map<std::string, std::string> sources_;
};

} // namespace

RubyVM::RubyVM(RubyBackend backend) : backend_(backend) {}

RubyVM::~RubyVM() = default;

std::unique_ptr<RubyVM> RubyVM::create(RubyBackend backend) {
    switch (backend) {
    case RubyBackend::MRuby:
        core::log_warn("Ruby", "mruby backend not linked – using Stub");
        [[fallthrough]];
    case RubyBackend::Stub:
    default:
        return std::make_unique<StubRubyVM>();
    }
}

void RubyVM::define_function(HostFunction fn) {
    // replace if exists
    for (auto& h : host_functions_) {
        if (h.module == fn.module && h.name == fn.name) {
            h = std::move(fn);
            return;
        }
    }
    host_functions_.push_back(std::move(fn));
}

HostFunction* RubyVM::find_host(std::string_view module, std::string_view name) {
    for (auto& h : host_functions_) {
        if (h.module == module && h.name == name) return &h;
    }
    return nullptr;
}

const HostFunction* RubyVM::find_host(std::string_view module, std::string_view name) const {
    for (const auto& h : host_functions_) {
        if (h.module == module && h.name == name) return &h;
    }
    return nullptr;
}

EvalResult RubyVM::call_host(std::string_view module,
                             std::string_view name,
                             const std::vector<std::string>& args) {
    auto* h = find_host(module, name);
    if (!h || !h->fn) {
        EvalResult r;
        r.ok = false;
        r.error = "Undefined host method " + std::string(module) + "." + std::string(name);
        last_error_ = r.error;
        return r;
    }
    try {
        EvalResult r;
        r.ok = true;
        r.value = h->fn(args);
        return r;
    } catch (const std::exception& ex) {
        EvalResult r;
        r.ok = false;
        r.error = ex.what();
        last_error_ = r.error;
        return r;
    }
}

EvalResult RubyVM::reload_file(std::string_view path) {
    core::log_info("Ruby", std::string("Hot-reload: ") + std::string(path));
    return load_file(path);
}

std::string engine_api_bootstrap_source() {
    return R"RUBY(
# Aether Engine Ruby API Bootstrap (conceptual – fully original implementation)
# Modules mirror classic RPG script layers without using proprietary RGSS code.

module Graphics
  def self.frame_rate; 60; end
  def self.frame_rate=(v); end
  def self.width; 1280; end
  def self.height; 720; end
  def self.update; end
end

module Audio
  def self.bgm_play(name, volume = 80, pitch = 100, pos = 0); end
  def self.bgm_stop(fade = 0); end
  def self.bgs_play(name, volume = 64, pitch = 100); end
  def self.bgs_stop(fade = 0); end
  def self.me_play(name, volume = 80, pitch = 100); end
  def self.me_stop(fade = 0); end
  def self.se_play(name, volume = 80, pitch = 100); end
  def self.se_stop; end
end

module Input
  def self.press?(sym); false; end
  def self.trigger?(sym); false; end
  def self.repeat?(sym); false; end
end

module SceneManager
  def self.goto(scene); end
  def self.call(scene); end
  def self.return; end
end

module Player
  def self.transfer(map_id, x, y, z = 0, direction = 2); end
  def self.x; 0; end
  def self.y; 0; end
  def self.z; 0; end
end

module NPC
  def self.find(name); nil; end
end

module Enemy
  def self.spawn(id, x, y, z = 0); end
end

module Camera
  def self.move_to(x, y, z, frames = 0); end
  def self.shake(power, frames); end
end

module Weather
  def self.set(type, power: 5, frames: 0); end
  def self.clear(frames = 0); end
end

module Inventory
  def self.gain(item, amount = 1); end
  def self.lose(item, amount = 1); end
  def self.count(item); 0; end
end

module Quest
  def self.start(id); end
  def self.complete(id); end
  def self.active?(id); false; end
end

module Dialogue
  def self.start(id); end
  def self.choice(*options); 0; end
end

module Map
  def self.id; 0; end
  def self.tint(r, g, b, frames = 0); end
  def self.scroll(dx, dy, frames = 0); end
end
)RUBY";
}

} // namespace aether::ruby
