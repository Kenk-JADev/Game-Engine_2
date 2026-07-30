/**
 * @file gltf_script_test.cpp
 * @brief Minimal glTF load (if sample) + script tokenizer.
 */
#include <aether/res/gltf_loader.hpp>
#include <aether/res/resource_module.hpp>

// Editor highlighter is not in engine – duplicate tiny check via writing a temp gltf
// and testing resource path. Script highlighter tests via include from editor path is messy.
// We only test gltf loader + that mesh load tries gltf extension.

#include <nlohmann/json.hpp>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

using namespace aether;
using namespace aether::res;
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

// Minimal valid-ish glTF 2.0 with one triangle (embedded buffer)
static void write_min_gltf(const fs::path& path) {
    // positions: (0,0,0) (1,0,0) (0,1,0) as float32 little endian base64
    // Use URI buffer file next to it for simplicity
    const auto bin = path.parent_path() / "tri.bin";
    {
        // 3 * vec3 f32 = 36 bytes positions + 3 * u16 indices = 6 (+pad)
        std::vector<unsigned char> buf;
        float pos[] = {0,0,0, 1,0,0, 0,1,0};
        buf.resize(36);
        std::memcpy(buf.data(), pos, 36);
        std::uint16_t idx[] = {0,1,2};
        buf.resize(36 + 6);
        std::memcpy(buf.data() + 36, idx, 6);
        std::ofstream o(bin, std::ios::binary);
        o.write(reinterpret_cast<const char*>(buf.data()), static_cast<std::streamsize>(buf.size()));
    }
    nlohmann::json j = {
        {"asset", {{"version", "2.0"}}},
        {"buffers", {{{"byteLength", 42}, {"uri", "tri.bin"}}}},
        {"bufferViews",
         {{{"buffer", 0}, {"byteOffset", 0}, {"byteLength", 36}, {"target", 34962}},
          {{"buffer", 0}, {"byteOffset", 36}, {"byteLength", 6}, {"target", 34963}}}},
        {"accessors",
         {{{"bufferView", 0},
           {"componentType", 5126},
           {"count", 3},
           {"type", "VEC3"},
           {"max", {1, 1, 0}},
           {"min", {0, 0, 0}}},
          {{"bufferView", 1},
           {"componentType", 5123},
           {"count", 3},
           {"type", "SCALAR"}}}},
        {"meshes",
         {{{"primitives",
            {{{"attributes", {{"POSITION", 0}}}, {"indices", 1}, {"mode", 4}}}}}}},
        {"nodes", {{{"mesh", 0}}}},
        {"scenes", {{{"nodes", {0}}}}},
        {"scene", 0}};
    std::ofstream o(path);
    o << j.dump(2);
}

int main() {
    const fs::path dir = fs::temp_directory_path() / "aether_gltf_test";
    fs::create_directories(dir);
    const fs::path gltf = dir / "tri.gltf";
    write_min_gltf(gltf);

    auto mesh = load_gltf_mesh(gltf);
    CHECK(mesh != nullptr);
    if (mesh) {
        CHECK(mesh->lod_count() >= 1);
        CHECK(mesh->lod(0).vertices.size() == 3);
        CHECK(mesh->lod(0).indices.size() == 3);
    }

    // ResourceManager path
    ResourceManager res;
    res.mount("m", dir);
    auto m2 = res.load_mesh("m/tri.gltf");
    CHECK(m2.is_ok());
    if (m2) {
        CHECK(m2.value()->lod(0).vertices.size() == 3);
    }

    fs::remove_all(dir);

    if (g_failures == 0) {
        std::puts("OK: gltf_script_test passed");
        return 0;
    }
    std::fprintf(stderr, "FAIL: %d\n", g_failures);
    return 1;
}
