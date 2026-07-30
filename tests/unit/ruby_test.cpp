/**
 * @file ruby_test.cpp
 */
#include <aether/ruby/ruby_module.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>

using namespace aether::ruby;
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
    // Prefer stub for deterministic unit tests of the lightweight evaluator.
    // mruby is covered in CI integration when AETHER_WITH_MRUBY=ON.
    auto vm = RubyVM::create(RubyBackend::Stub);
    CHECK(vm != nullptr);
    CHECK(vm->backend() == RubyBackend::Stub);
    vm->define_engine_api();

    auto r = vm->eval("return 42");
    CHECK(r.ok);
    CHECK(r.value == "42");

    r = vm->eval("$hero = \"Alice\"");
    CHECK(r.ok);
    CHECK(vm->get_global("hero") == "Alice");

    r = vm->eval("Audio.bgm_play(\"Theme1\", 80, 100)");
    CHECK(r.ok);

    r = vm->eval("Graphics.width");
    CHECK(r.ok);
    CHECK(r.value == "1280");

    r = vm->call_host("NPC", "find", {"elder"});
    CHECK(r.ok);
    CHECK(r.value.find("elder") != std::string::npos);

    // load file + reload
    const fs::path dir = fs::temp_directory_path() / "aether_ruby_test";
    fs::create_directories(dir);
    const fs::path script = dir / "main.rb";
    {
        std::ofstream(script) << "$boot = \"ok\"\n";
    }
    r = vm->load_file(script.string());
    CHECK(r.ok);
    CHECK(vm->get_global("boot") == "ok");
    {
        std::ofstream(script) << "$boot = \"reloaded\"\n";
    }
    r = vm->reload_file(script.string());
    CHECK(r.ok);
    CHECK(vm->get_global("boot") == "reloaded");

    CHECK(!engine_api_bootstrap_source().empty());

    fs::remove_all(dir);

    if (g_failures == 0) {
        std::puts("OK: ruby_test passed");
        return 0;
    }
    std::fprintf(stderr, "FAIL: %d check(s) failed\n", g_failures);
    return 1;
}
