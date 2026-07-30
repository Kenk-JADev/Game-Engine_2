/**
 * @file project_test.cpp
 */
#include <aether/shared/project_descriptor.hpp>

#include <cstdio>
#include <filesystem>

using namespace aether::shared;
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
    // Load sample
    ProjectDescriptor d;
    std::string err;
    const fs::path sample = fs::path("samples") / "demo_project";
    // may be relative to build/bin – try repo root paths
    fs::path path = sample;
    if (!fs::exists(path / "project.json")) {
        path = fs::path("..") / ".." / "samples" / "demo_project";
    }
    if (!fs::exists(path / "project.json")) {
        path = fs::path("/home/user/Game-Engine_2/samples/demo_project");
    }
    CHECK(load_project_descriptor(path, d, &err));
    CHECK(d.name == "Aether Demo");
    CHECK(d.graphics.width == 1280);
    CHECK(d.scripts.entry == "scripts/main.rb");

    const fs::path tmp = fs::temp_directory_path() / "aether_proj_test" / "project.json";
    fs::create_directories(tmp.parent_path());
    d.name = "Roundtrip";
    CHECK(save_project_descriptor(tmp, d, &err));

    ProjectDescriptor d2;
    CHECK(load_project_descriptor(tmp, d2, &err));
    CHECK(d2.name == "Roundtrip");

    fs::remove_all(tmp.parent_path());

    if (g_failures == 0) {
        std::puts("OK: project_test passed");
        return 0;
    }
    std::fprintf(stderr, "FAIL: %d check(s) failed\n", g_failures);
    return 1;
}
