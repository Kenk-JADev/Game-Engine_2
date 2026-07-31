/**
 * @file fbx_loader_test.cpp
 * @brief Tests für den eigenständigen FBX-Reader (ASCII + binär 7.x).
 *
 * Enthält einen kleinen Binär-FBX-Encoder (nur Testcode), um den
 * Binär-Parser deterministisch zu testen.
 */
#include <aether/res/fbx_loader.hpp>
#include <aether/res/resource_module.hpp>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
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

// =============================================================================
// ASCII-Fixture: Dreieck mit Normalen/UVs + Model-Translation
// =============================================================================

static void write_ascii_triangle(const fs::path& path) {
    std::ofstream o(path);
    o << R"(FBXHeaderExtension:  {
	FBXHeaderVersion: 1003
	FBXVersion: 7400
	Creator: "Aether Test"
}
GlobalSettings:  {
	Version: 1000
}
Definitions:  {
	Version: 100
	Count: 2
	ObjectType: "Geometry" {
		Count: 1
	}
	ObjectType: "Model" {
		Count: 1
	}
}
Objects:  {
	Geometry: 1000, "Geometry::Tri", "Mesh" {
		Vertices: *9 {
			a: 0,0,0, 1,0,0, 0,1,0
		}
		PolygonVertexIndex: *3 {
			a: 0,1,-3
		}
		LayerElementNormal: 0 {
			Version: 101
			Name: ""
			MappingInformationType: "ByPolygonVertex"
			ReferenceInformationType: "Direct"
			Normals: *9 {
				a: 0,0,1, 0,0,1, 0,0,1
			}
		}
		LayerElementUV: 0 {
			Version: 101
			Name: ""
			MappingInformationType: "ByPolygonVertex"
			ReferenceInformationType: "Direct"
			UV: *6 {
				a: 0,0, 1,0,
				0,1
			}
		}
	}
	Model: 2000, "Model::Tri", "Mesh" {
		Version: 232
		Properties70:  {
			P: "Lcl Translation", "Lcl Translation", "", "A", 2, 3, 4
		}
		Shading: T
		Culling: "CullingOff"
	}
}
Connections:  {
	C: "OO", 2000, 1000
}
)";
}

// =============================================================================
// Binär-Fixture: minimaler FBX-7400-Encoder (nur Testcode)
// =============================================================================

template <typename T>
static void append_le(std::vector<u8>& out, T v) {
    static_assert(std::is_integral<T>::value, "integral required");
    using U = typename std::make_unsigned<T>::type;
    U u = static_cast<U>(v);
    for (usize i = 0; i < sizeof(T); ++i) {
        out.push_back(static_cast<u8>((u >> (8 * i)) & 0xFFu));
    }
}

static void append_le(std::vector<u8>& out, double v) {
    u8 buf[sizeof(double)];
    std::memcpy(buf, &v, sizeof(double)); // x86-LE → direkt IEEE754-Bytes
    out.insert(out.end(), buf, buf + sizeof(double));
}

static std::vector<u8> bin_i64(i64 v) {
    std::vector<u8> out{'L'};
    append_le(out, v);
    return out;
}
static std::vector<u8> bin_i32(i32 v) {
    std::vector<u8> out{'I'};
    append_le(out, v);
    return out;
}
static std::vector<u8> bin_double(double v) {
    std::vector<u8> out{'D'};
    append_le(out, v);
    return out;
}
static std::vector<u8> bin_str(const std::string& s) {
    std::vector<u8> out{'S'};
    append_le(out, static_cast<u32>(s.size()));
    out.insert(out.end(), s.begin(), s.end());
    return out;
}
static std::vector<u8> bin_f64_array(const std::vector<double>& v) {
    std::vector<u8> out{'d'};
    append_le(out, static_cast<u32>(v.size()));
    append_le(out, static_cast<u32>(0)); // encoding: raw
    append_le(out, static_cast<u32>(v.size() * sizeof(double)));
    for (const double d : v) append_le(out, d);
    return out;
}
static std::vector<u8> bin_i32_array(const std::vector<i32>& v) {
    std::vector<u8> out{'i'};
    append_le(out, static_cast<u32>(v.size()));
    append_le(out, static_cast<u32>(0));
    append_le(out, static_cast<u32>(v.size() * sizeof(i32)));
    for (const i32 x : v) append_le(out, x);
    return out;
}

struct BinNode {
    std::string name;
    std::vector<std::vector<u8>> props; ///< jeweils inkl. Typ-Byte
    std::vector<BinNode> children;
};

static std::vector<u8> encode_node(const BinNode& n) {
    std::vector<u8> body;
    u32 prop_list_len = 0;
    for (const auto& p : n.props) prop_list_len += static_cast<u32>(p.size());
    for (const auto& c : n.children) {
        auto b = encode_node(c);
        body.insert(body.end(), b.begin(), b.end());
    }
    if (!n.children.empty()) {
        body.insert(body.end(), 17, static_cast<u8>(0)); // Null-Record
    }

    std::vector<u8> out;
    const u64 end_offset = 17ull + n.name.size() + prop_list_len + body.size();
    append_le(out, end_offset);
    append_le(out, static_cast<u32>(n.props.size()));
    append_le(out, prop_list_len);
    out.push_back(static_cast<u8>(n.name.size()));
    out.insert(out.end(), n.name.begin(), n.name.end());
    for (const auto& p : n.props) out.insert(out.end(), p.begin(), p.end());
    out.insert(out.end(), body.begin(), body.end());
    return out;
}

static void write_binary_quad(const fs::path& path) {
    std::vector<u8> data;
    // Header (23 Bytes) + Version 7400
    const char* magic = "Kaydara FBX Binary  ";
    data.insert(data.end(), magic, magic + 20);
    data.push_back(0x00);
    data.push_back(0x1A);
    data.push_back(0x00);
    append_le(data, static_cast<u32>(7400));

    BinNode header_ext;
    header_ext.name = "FBXHeaderExtension";
    header_ext.children = {
        {"FBXHeaderVersion", {bin_i32(1003)}, {}},
        {"FBXVersion", {bin_i32(7400)}, {}},
    };
    BinNode global;
    global.name = "GlobalSettings";
    BinNode documents;
    documents.name = "Documents";
    documents.children = {
        {"Count", {bin_i64(1)}, {}},
        {"Document", {bin_i64(1), bin_str("Doc"), bin_str("Scene")}, {}},
    };
    BinNode definitions;
    definitions.name = "Definitions";
    definitions.children = {
        {"Version", {bin_i32(100)}, {}},
        {"Count", {bin_i64(2)}, {}},
        {"ObjectType", {bin_str("Geometry")},
         {{"Count", {bin_i64(1)}, {}}}},
        {"ObjectType", {bin_str("Model")},
         {{"Count", {bin_i64(1)}, {}}}},
    };

    // Quad in der XY-Ebene
    const std::vector<double> verts = {0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0};
    const std::vector<i32> poly = {0, 1, 2, -4}; // Ende nach 4 Ecken
    const std::vector<double> normals = {0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1};
    const std::vector<double> uvs = {0, 0, 1, 0, 1, 1, 0, 1};

    BinNode layer_normal;
    layer_normal.name = "LayerElementNormal";
    layer_normal.props = {bin_i32(0)};
    layer_normal.children = {
        {"Version", {bin_i32(101)}, {}},
        {"Name", {bin_str("")}, {}},
        {"MappingInformationType", {bin_str("ByPolygonVertex")}, {}},
        {"ReferenceInformationType", {bin_str("Direct")}, {}},
        {"Normals", {bin_f64_array(normals)}, {}},
    };
    BinNode layer_uv;
    layer_uv.name = "LayerElementUV";
    layer_uv.props = {bin_i32(0)};
    layer_uv.children = {
        {"Version", {bin_i32(101)}, {}},
        {"Name", {bin_str("")}, {}},
        {"MappingInformationType", {bin_str("ByPolygonVertex")}, {}},
        {"ReferenceInformationType", {bin_str("Direct")}, {}},
        {"UV", {bin_f64_array(uvs)}, {}},
    };

    BinNode geometry;
    geometry.name = "Geometry";
    geometry.props = {bin_i64(1000), bin_str("Geometry::Quad"), bin_str("Mesh")};
    geometry.children = {
        {"Vertices", {bin_f64_array(verts)}, {}},
        {"PolygonVertexIndex", {bin_i32_array(poly)}, {}},
        layer_normal,
        layer_uv,
    };

    BinNode p_translation;
    p_translation.name = "P";
    p_translation.props = {bin_str("Lcl Translation"), bin_str("Lcl Translation"),
                           bin_str(""), bin_str("A"), bin_double(1.0),
                           bin_double(2.0), bin_double(3.0)};
    BinNode properties70;
    properties70.name = "Properties70";
    properties70.children = {p_translation};

    BinNode model;
    model.name = "Model";
    model.props = {bin_i64(2000), bin_str("Model::Quad"), bin_str("Mesh")};
    model.children = {
        {"Version", {bin_i32(232)}, {}},
        properties70,
        {"Shading", {bin_str("T")}, {}},
    };

    BinNode objects;
    objects.name = "Objects";
    objects.children = {geometry, model};

    BinNode connection;
    connection.name = "C";
    connection.props = {bin_str("OO"), bin_i64(2000), bin_i64(1000)};
    BinNode connections;
    connections.name = "Connections";
    connections.children = {connection};

    for (const BinNode* root :
         {&header_ext, &global, &documents, &definitions, &objects, &connections}) {
        auto b = encode_node(*root);
        data.insert(data.end(), b.begin(), b.end());
    }
    data.insert(data.end(), 17, static_cast<u8>(0)); // Null-Record am Dateiende

    std::ofstream o(path, std::ios::binary);
    o.write(reinterpret_cast<const char*>(data.data()),
            static_cast<std::streamsize>(data.size()));
}

// =============================================================================

static bool near(f32 a, f32 b, f32 eps = 1.0e-4f) {
    return std::fabs(a - b) < eps;
}

static bool near_vec(const render::Vec3& a, const render::Vec3& b) {
    return near(a.x, b.x) && near(a.y, b.y) && near(a.z, b.z);
}

int main() {
    const fs::path root = fs::temp_directory_path() / "aether_fbx_test";
    fs::remove_all(root);
    fs::create_directories(root);

    // --- ASCII-Dreieck -------------------------------------------------------
    const fs::path ascii = root / "tri.fbx";
    write_ascii_triangle(ascii);
    auto mesh = load_fbx_mesh(ascii);
    CHECK(mesh != nullptr);
    if (mesh) {
        CHECK(mesh->lod_count() >= 1);
        const auto& lod = mesh->lod(0);
        CHECK(lod.vertices.size() == 3);
        CHECK(lod.indices.size() == 3);
        if (lod.vertices.size() == 3) {
            // Translation (2,3,4) angewendet
            CHECK(near_vec(lod.vertices[0].position, {2, 3, 4}));
            CHECK(near_vec(lod.vertices[1].position, {3, 3, 4}));
            CHECK(near_vec(lod.vertices[2].position, {2, 4, 4}));
            // Normale + UV
            CHECK(near_vec(lod.vertices[0].normal, {0, 0, 1}));
            CHECK(near(lod.vertices[0].uv.x, 0.0f) && near(lod.vertices[0].uv.y, 0.0f));
            CHECK(near(lod.vertices[1].uv.x, 1.0f) && near(lod.vertices[1].uv.y, 0.0f));
            CHECK(near(lod.vertices[2].uv.x, 0.0f) && near(lod.vertices[2].uv.y, 1.0f));
        }
        CHECK(mesh->bounds().radius() > 0.0f);
    }

    // --- Binär-Quad (Fan-Triangulation → 2 Dreiecke) -------------------------
    const fs::path bin = root / "quad.fbx";
    write_binary_quad(bin);
    auto mesh2 = load_fbx_mesh(bin);
    CHECK(mesh2 != nullptr);
    if (mesh2) {
        const auto& lod = mesh2->lod(0);
        CHECK(lod.vertices.size() == 6);
        CHECK(lod.indices.size() == 6);
        if (lod.vertices.size() == 6) {
            // Triangle 1: (0,0,0)(1,0,0)(1,1,0) + Translation (1,2,3)
            CHECK(near_vec(lod.vertices[0].position, {1, 2, 3}));
            CHECK(near_vec(lod.vertices[1].position, {2, 2, 3}));
            CHECK(near_vec(lod.vertices[2].position, {2, 3, 3}));
            // Triangle 2: (0,0,0)(1,1,0)(0,1,0) + Translation
            CHECK(near_vec(lod.vertices[3].position, {1, 2, 3}));
            CHECK(near_vec(lod.vertices[4].position, {2, 3, 3}));
            CHECK(near_vec(lod.vertices[5].position, {1, 3, 3}));
            // Normalen + UVs
            CHECK(near_vec(lod.vertices[0].normal, {0, 0, 1}));
            CHECK(near(lod.vertices[4].uv.x, 1.0f) && near(lod.vertices[4].uv.y, 1.0f));
        }
    }

    // --- ResourceManager-Integration (.fbx) ----------------------------------
    ResourceManager res;
    res.mount("m", root);
    auto m3 = res.load_mesh("m/tri.fbx");
    CHECK(m3.is_ok());
    if (m3) {
        CHECK(m3.value()->lod(0).vertices.size() == 3);
    }

    // --- Fehlerpfade ----------------------------------------------------------
    CHECK(load_fbx_mesh(root / "does_not_exist.fbx") == nullptr);
    {
        const fs::path junk = root / "junk.fbx";
        std::ofstream(root / "junk.fbx", std::ios::binary) << "not an fbx file";
        CHECK(load_fbx_mesh(junk) == nullptr);
    }

    fs::remove_all(root);

    if (g_failures == 0) {
        std::puts("OK: fbx_loader_test passed");
        return 0;
    }
    std::fprintf(stderr, "FAIL: %d\n", g_failures);
    return 1;
}
