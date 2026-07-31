/**
 * @file fbx_loader.cpp
 * @brief Eigenständiger FBX-Reader (ASCII + binär 7.x) → render::Mesh.
 *
 * Implementierungsnotizen:
 *  - Binär-Format: Node-Records (EndOffset u64, NumProperties u32,
 *    PropertyListLen u32, NameLen u8, Name, Properties, Kinder).
 *  - ASCII-Format: rekursiver, zeilenbasierter Parser; Arrays als
 *    `Name: *N { a: ... }`-Blöcke (Werte können über Zeilen laufen).
 *  - Beide Formate werden in einen gemeinsamen FbxNode-Baum überführt;
 *    die Geometrie-Extraktion arbeitet nur auf diesem Baum.
 */
#include <aether/res/fbx_loader.hpp>

#include <aether/core/logger.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace aether::res {
namespace {

// =============================================================================
// Generischer FBX-Baum
// =============================================================================

struct FbxValue {
    enum class Kind {
        None,
        Int,          ///< 'L','I','Y' → i64
        Double,       ///< 'D','F' → f64
        Bool,         ///< 'C'
        String,       ///< 'S'
        FloatArray,   ///< 'f'
        DoubleArray,  ///< 'd'
        IntArray,     ///< 'i'
        Int64Array,   ///< 'l'
    };

    Kind kind = Kind::None;
    i64 i = 0;
    double d = 0.0;
    bool b = false;
    std::string s;
    std::vector<float> fa;
    std::vector<double> da;
    std::vector<i32> ia;
    std::vector<i64> la;

    [[nodiscard]] std::string to_string() const;
    [[nodiscard]] double to_double() const noexcept;
    [[nodiscard]] i64 to_int() const noexcept;
};

std::string FbxValue::to_string() const {
    switch (kind) {
    case Kind::None: return "";
    case Kind::Int: return std::to_string(i);
    case Kind::Double: {
        std::ostringstream ss;
        ss.precision(9);
        ss << d;
        return ss.str();
    }
    case Kind::Bool: return b ? "1" : "0";
    case Kind::String: return s;
    default: return "";
    }
}

double FbxValue::to_double() const noexcept {
    switch (kind) {
    case Kind::Int: return static_cast<double>(i);
    case Kind::Double: return d;
    case Kind::Bool: return b ? 1.0 : 0.0;
    case Kind::String:
        try {
            return std::stod(s);
        } catch (...) {
            return 0.0;
        }
    default: return 0.0;
    }
}

i64 FbxValue::to_int() const noexcept {
    switch (kind) {
    case Kind::Int: return i;
    case Kind::Double: return static_cast<i64>(d);
    case Kind::Bool: return b ? 1 : 0;
    case Kind::String:
        try {
            return std::stoll(s);
        } catch (...) {
            return 0;
        }
    default: return 0;
    }
}

struct FbxNode {
    std::string name;
    std::vector<FbxValue> props;
    std::vector<FbxNode> children;

    [[nodiscard]] const FbxNode* find_child(std::string_view n) const {
        for (const auto& c : children) {
            if (c.name == n) return &c;
        }
        return nullptr;
    }
    [[nodiscard]] const FbxValue* prop(usize index) const {
        return index < props.size() ? &props[index] : nullptr;
    }
    [[nodiscard]] std::string prop_str(usize index, std::string_view dflt = "") const {
        const FbxValue* p = prop(index);
        return p ? p->to_string() : std::string(dflt);
    }
};

// =============================================================================
// Binär-Parser
// =============================================================================

class BinaryReader {
public:
    BinaryReader(const std::vector<u8>& data, usize offset = 0)
        : data_(data), pos_(offset) {}

    [[nodiscard]] usize position() const noexcept { return pos_; }
    [[nodiscard]] usize total() const noexcept { return data_.size(); }
    [[nodiscard]] usize remaining() const noexcept { return data_.size() - pos_; }

    bool read_bytes(void* out, usize n) {
        if (remaining() < n) return false;
        std::memcpy(out, data_.data() + pos_, n);
        pos_ += n;
        return true;
    }
    bool read_u8(u8& v) { return read_bytes(&v, 1); }
    bool read_u32(u32& v) { return read_bytes(&v, 4); }
    bool read_u64(u64& v) { return read_bytes(&v, 8); }
    bool read_i16(i16& v) { return read_bytes(&v, 2); }
    bool read_i32(i32& v) { return read_bytes(&v, 4); }
    bool read_i64(i64& v) { return read_bytes(&v, 8); }
    bool read_f32(f32& v) { return read_bytes(&v, 4); }
    bool read_f64(f64& v) { return read_bytes(&v, 8); }

    std::string read_string(u32 len) {
        if (remaining() < len) return {};
        std::string s(reinterpret_cast<const char*>(data_.data() + pos_), len);
        pos_ += len;
        return s;
    }

    void seek(usize pos) noexcept { pos_ = pos; }

private:
    const std::vector<u8>& data_;
    usize pos_ = 0;
};

/**
 * @brief Liest einen Property-Wert (nach Typ-Byte).
 * @return false bei Lesefehler oder nicht unterstützter Kompression.
 */
bool read_property(BinaryReader& r, FbxValue& out) {
    u8 type = 0;
    if (!r.read_u8(type)) return false;

    switch (static_cast<char>(type)) {
    case 'Y': { // i16
        i16 v = 0;
        if (!r.read_i16(v)) return false;
        out.kind = FbxValue::Kind::Int;
        out.i = v;
        return true;
    }
    case 'C': { // 1-Byte bool
        u8 v = 0;
        if (!r.read_u8(v)) return false;
        out.kind = FbxValue::Kind::Bool;
        out.b = v != 0;
        return true;
    }
    case 'I': {
        i32 v = 0;
        if (!r.read_i32(v)) return false;
        out.kind = FbxValue::Kind::Int;
        out.i = v;
        return true;
    }
    case 'F': {
        f32 v = 0;
        if (!r.read_f32(v)) return false;
        out.kind = FbxValue::Kind::Double;
        out.d = v;
        return true;
    }
    case 'D': {
        f64 v = 0;
        if (!r.read_f64(v)) return false;
        out.kind = FbxValue::Kind::Double;
        out.d = v;
        return true;
    }
    case 'L': {
        i64 v = 0;
        if (!r.read_i64(v)) return false;
        out.kind = FbxValue::Kind::Int;
        out.i = v;
        return true;
    }
    case 'S': {
        u32 len = 0;
        if (!r.read_u32(len)) return false;
        out.kind = FbxValue::Kind::String;
        out.s = r.read_string(len);
        return out.s.size() == len;
    }
    case 'R': { // Raw – überspringen
        u32 len = 0;
        if (!r.read_u32(len)) return false;
        if (r.remaining() < len) return false;
        r.seek(r.position() + len);
        out.kind = FbxValue::Kind::None;
        return true;
    }
    case 'f':
    case 'd':
    case 'i':
    case 'l':
    case 'b': {
        u32 count = 0, encoding = 0, compressed = 0;
        if (!r.read_u32(count) || !r.read_u32(encoding) || !r.read_u32(compressed)) {
            return false;
        }
        if (encoding != 0) {
            core::log_warn("FBX", "compressed array properties are not supported – "
                                  "please export FBX without compression");
            return false;
        }
        const u32 elem = (type == 'f' || type == 'i') ? 4u : (type == 'b') ? 1u : 8u;
        const usize need = static_cast<usize>(count) * elem;
        if (compressed != need || r.remaining() < need) {
            return false;
        }
        if (type == 'f') {
            out.kind = FbxValue::Kind::FloatArray;
            out.fa.resize(count);
            for (u32 i = 0; i < count; ++i) {
                f32 v = 0;
                if (!r.read_f32(v)) return false;
                out.fa[i] = v;
            }
        } else if (type == 'd') {
            out.kind = FbxValue::Kind::DoubleArray;
            out.da.resize(count);
            for (u32 i = 0; i < count; ++i) {
                f64 v = 0;
                if (!r.read_f64(v)) return false;
                out.da[i] = v;
            }
        } else if (type == 'i') {
            out.kind = FbxValue::Kind::IntArray;
            out.ia.resize(count);
            for (u32 i = 0; i < count; ++i) {
                i32 v = 0;
                if (!r.read_i32(v)) return false;
                out.ia[i] = v;
            }
        } else if (type == 'l') {
            out.kind = FbxValue::Kind::Int64Array;
            out.la.resize(count);
            for (u32 i = 0; i < count; ++i) {
                i64 v = 0;
                if (!r.read_i64(v)) return false;
                out.la[i] = v;
            }
        } else { // 'b'
            out.kind = FbxValue::Kind::IntArray;
            out.ia.resize(count);
            for (u32 i = 0; i < count; ++i) {
                u8 v = 0;
                if (!r.read_u8(v)) return false;
                out.ia[i] = v ? 1 : 0;
            }
        }
        return true;
    }
    default:
        core::log_warn("FBX", "unsupported property type '" +
                                  std::string(1, static_cast<char>(type)) + "'");
        return false;
    }
}

/**
 * @brief Liest rekursiv Node-Records.
 * @param end Absolute End-Position des Eltern-Records (exklusiv).
 */
bool read_binary_nodes(BinaryReader& r, u64 end, std::vector<FbxNode>& out) {
    while (r.remaining() >= 17 && static_cast<u64>(r.position()) < end) {
        // WICHTIG: EndOffset ist RELATIV zum Record-Anfang (Offset des
        // EndOffset-Feldes), nicht absolut in der Datei.
        const u64 record_start = static_cast<u64>(r.position());
        u64 node_end = 0;
        u32 num_props = 0, prop_list_len = 0;
        u8 name_len = 0;
        if (!r.read_u64(node_end) || !r.read_u32(num_props) ||
            !r.read_u32(prop_list_len) || !r.read_u8(name_len)) {
            return false;
        }
        // Null-Record (Sentinel) → Ende der Geschwisterliste
        if (node_end == 0 && num_props == 0 && prop_list_len == 0 && name_len == 0) {
            return true;
        }
        const u64 node_end_abs = record_start + node_end;
        if (node_end_abs > r.total() || name_len > r.remaining()) {
            return false;
        }

        FbxNode node;
        node.name = r.read_string(name_len);

        // Properties: exakt PropertyListLen Bytes, aber nie über node_end hinaus
        const u64 props_start = static_cast<u64>(r.position());
        const u64 props_end = std::min(props_start + prop_list_len, node_end_abs);
        for (u32 i = 0; i < num_props; ++i) {
            if (static_cast<u64>(r.position()) >= props_end) return false;
            FbxValue v;
            if (!read_property(r, v)) return false;
            node.props.push_back(std::move(v));
        }
        // Kinder
        if (static_cast<u64>(r.position()) < node_end_abs) {
            if (!read_binary_nodes(r, node_end_abs, node.children)) return false;
        }
        r.seek(static_cast<usize>(node_end_abs));
        out.push_back(std::move(node));
    }
    return true;
}

// =============================================================================
// ASCII-Parser (rekursiv, zeilenbasiert)
// =============================================================================

std::string ascii_trim(std::string_view s) {
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r')) {
        s.remove_prefix(1);
    }
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r')) {
        s.remove_suffix(1);
    }
    return std::string(s);
}

/** @brief Zerlegt eine Werteliste (Zahlen / Strings / Bool) in FbxValue-Props. */
void ascii_split_values(const std::string& text, std::vector<FbxValue>& props) {
    std::string cur;
    bool in_string = false;
    auto flush = [&] {
        std::string tok = ascii_trim(cur);
        cur.clear();
        if (tok.empty()) return;
        FbxValue v;
        if (tok.front() == '"' && tok.back() == '"' && tok.size() >= 2) {
            v.kind = FbxValue::Kind::String;
            v.s = tok.substr(1, tok.size() - 2);
        } else if (tok == "T" || tok == "Y") {
            v.kind = FbxValue::Kind::Bool;
            v.b = true;
        } else if (tok == "F" || tok == "N") {
            v.kind = FbxValue::Kind::Bool;
            v.b = false;
        } else if (tok.find_first_of(".eE") != std::string::npos) {
            try {
                v.kind = FbxValue::Kind::Double;
                v.d = std::stod(tok);
            } catch (...) {
                v.kind = FbxValue::Kind::String;
                v.s = tok;
            }
        } else {
            try {
                v.kind = FbxValue::Kind::Int;
                v.i = std::stoll(tok);
            } catch (...) {
                v.kind = FbxValue::Kind::String;
                v.s = tok;
            }
        }
        props.push_back(std::move(v));
    };

    for (usize i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (c == '"') {
            in_string = !in_string;
            cur.push_back(c);
        } else if (c == ',' && !in_string) {
            flush();
        } else {
            cur.push_back(c);
        }
    }
    flush();
}

struct AsciiParser {
    explicit AsciiParser(std::istream& in) {
        std::string line;
        while (std::getline(in, line)) {
            std::string t = ascii_trim(line);
            if (t.empty() || t.starts_with(';') || t.starts_with('#')) continue;
            lines.push_back(std::move(t));
        }
    }

    [[nodiscard]] bool at_end() const noexcept { return pos >= lines.size(); }
    [[nodiscard]] const std::string& cur() const noexcept { return lines[pos]; }
    void advance() noexcept { ++pos; }

    std::vector<std::string> lines;
    usize pos = 0;
};

void ascii_parse_array_block(AsciiParser& p, std::vector<FbxValue>& out) {
    // Nach "Name: *N {" – sammle "a: ..." Zeilen bis "}"
    // WICHTIG: das abschließende "}" wird IMMER konsumiert (auch bei
    // einzeiligen Arrays), sonst schließt es fälschlich den Eltern-Knoten.
    std::vector<FbxValue> scalars;
    std::string joined;
    while (!p.at_end()) {
        const std::string t = p.cur();
        p.advance();
        if (t == "}") break;
        std::string v = t;
        if (v.starts_with("a:")) {
            v = ascii_trim(std::string_view(v).substr(2));
        }
        const bool continues = (!v.empty() && v.back() == ',');
        joined += v;
        if (!continues) {
            // Wertezeile abgeschlossen → zugehöriges "}" konsumieren
            if (!p.at_end() && p.cur() == "}") p.advance();
            break;
        }
    }
    ascii_split_values(joined, scalars);
    if (scalars.empty()) return;

    // Einheitlich als ein Array-Property ablegen (wie im Binärformat)
    bool all_int = true;
    for (const auto& s : scalars) {
        if (s.kind != FbxValue::Kind::Int && s.kind != FbxValue::Kind::Bool) {
            all_int = false;
            break;
        }
    }
    FbxValue arr;
    if (all_int) {
        arr.kind = FbxValue::Kind::IntArray;
        arr.ia.reserve(scalars.size());
        for (const auto& s : scalars) arr.ia.push_back(static_cast<i32>(s.to_int()));
    } else {
        arr.kind = FbxValue::Kind::DoubleArray;
        arr.da.reserve(scalars.size());
        for (const auto& s : scalars) arr.da.push_back(s.to_double());
    }
    out.push_back(std::move(arr));
}

void ascii_parse_children(AsciiParser& p, std::vector<FbxNode>& out);

/** @brief Parst "Name: rest" an aktueller Position. */
bool ascii_parse_one(AsciiParser& p, FbxNode& out) {
    if (p.at_end()) return false;
    const std::string t = p.cur();
    p.advance();

    // Ersten Doppelpunkt finden (nicht innerhalb eines Strings)
    usize colon = std::string::npos;
    bool in_str = false;
    for (usize i = 0; i < t.size(); ++i) {
        if (t[i] == '"') {
            in_str = !in_str;
        } else if (t[i] == ':' && !in_str) {
            colon = i;
            break;
        }
    }
    if (colon == std::string::npos) return false;

    out.name = ascii_trim(std::string_view(t).substr(0, colon));
    std::string rest = ascii_trim(std::string_view(t).substr(colon + 1));

    if (rest == "{") {
        ascii_parse_children(p, out.children);
    } else if (rest.starts_with("*")) {
        // Array-Block "Name: *N {" – Werte folgen in "a:"-Zeilen
        ascii_parse_array_block(p, out.props);
    } else if (rest.size() >= 2 && rest.back() == '{') {
        const std::string values = ascii_trim(std::string_view(rest).substr(0, rest.size() - 1));
        ascii_split_values(values, out.props);
        ascii_parse_children(p, out.children);
    } else {
        ascii_split_values(rest, out.props);
    }
    return true;
}

void ascii_parse_children(AsciiParser& p, std::vector<FbxNode>& out) {
    while (!p.at_end()) {
        if (p.cur() == "}") {
            p.advance();
            return;
        }
        FbxNode child;
        if (!ascii_parse_one(p, child)) {
            p.advance(); // unerwartete Zeile überspringen
            continue;
        }
        out.push_back(std::move(child));
    }
}

std::vector<FbxNode> parse_ascii(std::istream& in) {
    AsciiParser p(in);
    std::vector<FbxNode> roots;
    while (!p.at_end()) {
        if (p.cur() == "}") {
            p.advance();
            continue;
        }
        FbxNode node;
        if (ascii_parse_one(p, node)) {
            roots.push_back(std::move(node));
        } else {
            p.advance();
        }
    }
    return roots;
}

// =============================================================================
// Gemeinsame Extraktion
// =============================================================================

/** @brief Zugriff auf ein Zahlen-Array als f64 (Skalare werden 1-Element-Arrays). */
bool value_as_doubles(const FbxValue& v, std::vector<double>& out) {
    switch (v.kind) {
    case FbxValue::Kind::DoubleArray: out = v.da; return true;
    case FbxValue::Kind::FloatArray: out.assign(v.fa.begin(), v.fa.end()); return true;
    case FbxValue::Kind::IntArray: out.assign(v.ia.begin(), v.ia.end()); return true;
    case FbxValue::Kind::Int64Array: out.assign(v.la.begin(), v.la.end()); return true;
    case FbxValue::Kind::Double: out = {v.d}; return true;
    case FbxValue::Kind::Int: out = {static_cast<double>(v.i)}; return true;
    case FbxValue::Kind::Bool: out = {v.b ? 1.0 : 0.0}; return true;
    default: return false;
    }
}

/** @brief Zugriff auf ein Zahlen-Array als i32 (Skalare werden 1-Element-Arrays). */
bool value_as_ints(const FbxValue& v, std::vector<i32>& out) {
    switch (v.kind) {
    case FbxValue::Kind::IntArray: out = v.ia; return true;
    case FbxValue::Kind::Int64Array: out.assign(v.la.begin(), v.la.end()); return true;
    case FbxValue::Kind::Int: out = {static_cast<i32>(v.i)}; return true;
    case FbxValue::Kind::Double: out = {static_cast<i32>(v.d)}; return true;
    case FbxValue::Kind::Bool: out = {v.b ? 1 : 0}; return true;
    default: return false;
    }
}

struct LayerData {
    std::string mapping = "ByPolygonVertex";
    std::string reference = "Direct";
    std::vector<double> values;
    std::vector<i32> index;
};

LayerData parse_layer(const FbxNode& layer) {
    LayerData ld;
    if (const FbxNode* m = layer.find_child("MappingInformationType")) {
        ld.mapping = m->prop_str(0, "ByPolygonVertex");
    }
    if (const FbxNode* r = layer.find_child("ReferenceInformationType")) {
        ld.reference = r->prop_str(0, "Direct");
    }
    if (const FbxNode* n = layer.find_child("Normals")) {
        if (!n->props.empty()) (void)value_as_doubles(n->props.front(), ld.values);
        if (const FbxNode* ix = layer.find_child("NormalIndex")) {
            if (!ix->props.empty()) (void)value_as_ints(ix->props.front(), ld.index);
        }
    } else if (const FbxNode* u = layer.find_child("UV")) {
        if (!u->props.empty()) (void)value_as_doubles(u->props.front(), ld.values);
        if (const FbxNode* ix = layer.find_child("UVIndex")) {
            if (!ix->props.empty()) (void)value_as_ints(ix->props.front(), ld.index);
        }
    }
    return ld;
}

struct FbxGeometry {
    std::string id;
    std::string name;
    std::vector<double> vertices;
    std::vector<i32> polygon_vertex_index;
    LayerData normals;
    LayerData uvs;
    std::vector<std::string> material_names;
};

struct FbxModel {
    std::string id;
    std::string name;
    std::string geometry_id; ///< über Connections verknüpft
    render::Vec3 translation{0.0f};
    render::Vec3 rotation{0.0f}; ///< Grad (XYZ)
    render::Vec3 scaling{1.0f};
};

/** @brief Wendet Translation/Rotation/Scaling auf einen Punkt an (FBX XYZ-Grad). */
render::Vec3 apply_model_transform(const FbxModel& m, render::Vec3 p) {
    p.x *= m.scaling.x;
    p.y *= m.scaling.y;
    p.z *= m.scaling.z;

    constexpr double kDeg2Rad = 3.14159265358979323846 / 180.0;
    const double rx = m.rotation.x * kDeg2Rad;
    const double ry = m.rotation.y * kDeg2Rad;
    const double rz = m.rotation.z * kDeg2Rad;
    { // Rotation um Z
        const double c = std::cos(rz), s = std::sin(rz);
        const double x = p.x * c - p.y * s;
        const double y = p.x * s + p.y * c;
        p.x = static_cast<f32>(x);
        p.y = static_cast<f32>(y);
    }
    { // Rotation um X
        const double c = std::cos(rx), s = std::sin(rx);
        const double y = p.y * c - p.z * s;
        const double z = p.y * s + p.z * c;
        p.y = static_cast<f32>(y);
        p.z = static_cast<f32>(z);
    }
    { // Rotation um Y
        const double c = std::cos(ry), s = std::sin(ry);
        const double x = p.x * c + p.z * s;
        const double z = -p.x * s + p.z * c;
        p.x = static_cast<f32>(x);
        p.z = static_cast<f32>(z);
    }
    p.x += m.translation.x;
    p.y += m.translation.y;
    p.z += m.translation.z;
    return p;
}

/** @brief Berechnet flache Normalen für neu hinzugefügte Dreiecke. */
void compute_flat_normals(render::MeshLod& lod, usize tri_first_vertex,
                          usize tri_count) {
    for (usize t = 0; t < tri_count; ++t) {
        const usize i0 = tri_first_vertex + t * 3;
        const auto& a = lod.vertices[i0].position;
        const auto& b = lod.vertices[i0 + 1].position;
        const auto& c = lod.vertices[i0 + 2].position;
        render::Vec3 n = glm::cross(b - a, c - a);
        const f32 len = glm::length(n);
        if (len > 1.0e-6f) n = n / len;
        lod.vertices[i0].normal = n;
        lod.vertices[i0 + 1].normal = n;
        lod.vertices[i0 + 2].normal = n;
    }
}

/**
 * @brief Extrahiert ein render::Mesh aus allen Model-Geometrien.
 * @return nullptr bei leerer Geometrie
 */
std::shared_ptr<render::Mesh> build_mesh(const std::vector<FbxNode>& roots,
                                         const std::string& source_name) {
    // --- Geometrien + Modelle sammeln -------------------------------------
    std::unordered_map<std::string, FbxGeometry> geometries;
    std::unordered_map<std::string, FbxModel> models;

    const FbxNode* objects = nullptr;
    for (const auto& r : roots) {
        if (r.name == "Objects") {
            objects = &r;
            break;
        }
    }
    if (!objects) {
        core::log_warn("FBX", "no 'Objects' section found in '" + source_name + "'");
        return nullptr;
    }

    for (const auto& node : objects->children) {
        if (node.name == "Geometry" && node.props.size() >= 2) {
            FbxGeometry g;
            g.id = node.prop_str(0);
            g.name = node.prop_str(1);
            if (const FbxNode* v = node.find_child("Vertices")) {
                if (!v->props.empty()) (void)value_as_doubles(v->props.front(), g.vertices);
            }
            if (const FbxNode* p = node.find_child("PolygonVertexIndex")) {
                if (!p->props.empty()) (void)value_as_ints(p->props.front(),
                                                           g.polygon_vertex_index);
            }
            if (const FbxNode* nl = node.find_child("LayerElementNormal")) {
                g.normals = parse_layer(*nl);
            }
            if (const FbxNode* ul = node.find_child("LayerElementUV")) {
                g.uvs = parse_layer(*ul);
            }
            if (const FbxNode* mat = node.find_child("LayerElementMaterial")) {
                if (const FbxNode* mn = mat->find_child("Materials")) {
                    if (!mn->props.empty()) {
                        std::vector<i32> mi;
                        if (value_as_ints(mn->props.front(), mi)) {
                            g.material_names.reserve(mi.size());
                            for (const i32 m : mi) g.material_names.push_back(std::to_string(m));
                        }
                    }
                }
            }
            if (!g.id.empty() && !g.vertices.empty() && !g.polygon_vertex_index.empty()) {
                geometries.emplace(g.id, std::move(g));
            }
        } else if (node.name == "Model" && !node.props.empty()) {
            FbxModel m;
            m.id = node.prop_str(0);
            m.name = node.prop_str(1);
            if (const FbxNode* p70 = node.find_child("Properties70")) {
                // P: "Name", "Type", "Label", "Flags", v0, v1, v2  (7 Einträge)
                for (const auto& p : p70->children) {
                    if (p.name != "P" || p.props.size() < 7) continue;
                    const std::string key = p.prop_str(0);
                    const f32 a = static_cast<f32>(p.props[4].to_double());
                    const f32 b = static_cast<f32>(p.props[5].to_double());
                    const f32 c = static_cast<f32>(p.props[6].to_double());
                    if (key == "Lcl Translation") m.translation = {a, b, c};
                    else if (key == "Lcl Rotation") m.rotation = {a, b, c};
                    else if (key == "Lcl Scaling") m.scaling = {a, b, c};
                }
            }
            if (!m.id.empty()) models.emplace(m.id, std::move(m));
        }
    }

    // --- Connections: Model → Geometry -------------------------------------
    for (const auto& r : roots) {
        if (r.name != "Connections") continue;
        for (const auto& c : r.children) {
            if (c.name != "C" || c.props.size() < 3) continue;
            const std::string from = c.props[1].to_string();
            const std::string to = c.props[2].to_string();
            auto it = models.find(from);
            if (it != models.end() && geometries.count(to) > 0) {
                it->second.geometry_id = to;
            }
        }
    }

    // --- Meshdaten zusammenführen -------------------------------------------
    render::MeshLod lod;
    lod.max_distance = 1.0e9f;

    usize mesh_count = 0;
    for (auto& model : models) {
        if (model.second.geometry_id.empty()) continue;
        auto git = geometries.find(model.second.geometry_id);
        if (git == geometries.end()) continue;
        const FbxGeometry& g = git->second;

        const usize vert_count = g.vertices.size() / 3;
        if (vert_count == 0) continue;
        const bool has_normals = !g.normals.values.empty();
        const usize tri_first_vertex = lod.vertices.size();
        usize tri_count = 0;

        auto fetch_vec3 = [](const std::vector<double>& v, usize i) -> render::Vec3 {
            const usize idx = i * 3;
            if (idx + 2 >= v.size()) return {0.0f, 1.0f, 0.0f};
            return {static_cast<f32>(v[idx]), static_cast<f32>(v[idx + 1]),
                    static_cast<f32>(v[idx + 2])};
        };
        auto fetch_vec2 = [](const std::vector<double>& v, usize i) -> render::Vec2 {
            const usize idx = i * 2;
            if (idx + 1 >= v.size()) return {0.0f, 0.0f};
            return {static_cast<f32>(v[idx]), static_cast<f32>(v[idx + 1])};
        };
        auto fetch_pos = [&](i32 vi) -> render::Vec3 {
            if (vi < 0 || static_cast<usize>(vi) >= vert_count) {
                return {0.0f, 0.0f, 0.0f};
            }
            return fetch_vec3(g.vertices, static_cast<usize>(vi));
        };

        // Sequenzielle Polygon-Vertex-Nummer (für ByPolygonVertex / IndexToDirect)
        usize pv_seq = 0;
        usize pvi = 0;
        while (pvi < g.polygon_vertex_index.size()) {
            std::vector<i32> poly;
            while (pvi < g.polygon_vertex_index.size()) {
                const i32 raw = g.polygon_vertex_index[pvi++];
                if (raw < 0) {
                    poly.push_back(-raw - 1);
                    break;
                }
                poly.push_back(raw);
            }
            if (poly.size() < 3) {
                pv_seq += poly.size();
                continue;
            }

            auto normal_for = [&](usize corner, i32 vi) -> render::Vec3 {
                if (g.normals.reference == "IndexToDirect" && !g.normals.index.empty()) {
                    const usize ix = (pv_seq + corner < g.normals.index.size())
                                         ? static_cast<usize>(g.normals.index[pv_seq + corner])
                                         : 0u;
                    return fetch_vec3(g.normals.values, ix);
                }
                if (g.normals.mapping == "ByVertice") {
                    return fetch_vec3(g.normals.values, static_cast<usize>(vi));
                }
                if (g.normals.mapping == "AllSame") {
                    return fetch_vec3(g.normals.values, 0);
                }
                // ByPolygonVertex (Direct)
                return fetch_vec3(g.normals.values, pv_seq + corner);
            };
            auto uv_for = [&](usize corner, i32 vi) -> render::Vec2 {
                if (g.uvs.reference == "IndexToDirect" && !g.uvs.index.empty()) {
                    const usize ix = (pv_seq + corner < g.uvs.index.size())
                                         ? static_cast<usize>(g.uvs.index[pv_seq + corner])
                                         : 0u;
                    return fetch_vec2(g.uvs.values, ix);
                }
                if (g.uvs.mapping == "ByVertice") {
                    return fetch_vec2(g.uvs.values, static_cast<usize>(vi));
                }
                if (g.uvs.mapping == "AllSame") {
                    return fetch_vec2(g.uvs.values, 0);
                }
                return fetch_vec2(g.uvs.values, pv_seq + corner);
            };

            // Fan-Triangulation, pro Dreieck 3 eigene Vertices (flache Normalen möglich)
            for (usize k = 1; k + 1 < poly.size(); ++k) {
                const i32 corners[3] = {poly[0], poly[k], poly[k + 1]};
                const usize corner_idx[3] = {0, k, k + 1};
                for (int ci = 0; ci < 3; ++ci) {
                    render::Vertex v;
                    v.position = apply_model_transform(model.second, fetch_pos(corners[ci]));
                    v.normal = has_normals
                                   ? normal_for(corner_idx[static_cast<usize>(ci)],
                                                corners[ci])
                                   : render::Vec3{0.0f, 1.0f, 0.0f};
                    v.uv = uv_for(corner_idx[static_cast<usize>(ci)], corners[ci]);
                    v.color = {1.0f, 1.0f, 1.0f, 1.0f};
                    lod.vertices.push_back(v);
                }
                lod.indices.push_back(static_cast<u32>(tri_first_vertex + tri_count * 3));
                lod.indices.push_back(static_cast<u32>(tri_first_vertex + tri_count * 3 + 1));
                lod.indices.push_back(static_cast<u32>(tri_first_vertex + tri_count * 3 + 2));
                ++tri_count;
            }
            pv_seq += poly.size();
        }

        if (!has_normals && tri_count > 0) {
            compute_flat_normals(lod, tri_first_vertex, tri_count);
        }
        if (!g.material_names.empty()) {
            core::log_debug("FBX", "mesh '" + model.second.name + "' has " +
                                       std::to_string(g.material_names.size()) +
                                       " material slot(s)");
        }
        ++mesh_count;
    }
    if (lod.vertices.empty() || lod.indices.empty()) {
        core::log_warn("FBX", "no renderable geometry in '" + source_name + "'");
        return nullptr;
    }

    lod.recompute_bounds();
    auto mesh = std::make_shared<render::Mesh>(source_name);
    mesh->set_lod(0, std::move(lod));
    core::log_info("FBX", "imported " + std::to_string(mesh_count) + " mesh(es), " +
                              std::to_string(mesh->lod(0).vertices.size()) +
                              " vertices, " + std::to_string(mesh->lod(0).indices.size() / 3) +
                              " triangles from '" + source_name + "'");
    return mesh;
}

} // namespace

// =============================================================================
// Öffentliche API
// =============================================================================

std::shared_ptr<render::Mesh> load_fbx_mesh(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        core::log_error("FBX", "cannot open '" + path.string() + "'");
        return nullptr;
    }
    std::vector<u8> data((std::istreambuf_iterator<char>(in)),
                         std::istreambuf_iterator<char>());
    if (data.size() < 24) {
        core::log_error("FBX", "file too small: " + path.string());
        return nullptr;
    }

    const std::string src_name = path.filename().string();
    std::vector<FbxNode> roots;

    constexpr std::string_view kBinaryMagic = "Kaydara FBX Binary  ";
    const bool binary = data.size() >= kBinaryMagic.size() &&
                        std::memcmp(data.data(), kBinaryMagic.data(),
                                    kBinaryMagic.size()) == 0;

    if (binary) {
        u32 version = 0;
        std::memcpy(&version, data.data() + 23, 4);
        if (version < 7000 || version > 7400) {
            core::log_warn("FBX", "binary version " + std::to_string(version) +
                                      " – only 7.x (7000–7400) fully supported");
        }
        BinaryReader r(data, 27);
        if (!read_binary_nodes(r, data.size(), roots)) {
            core::log_error("FBX", "binary parse failed: " + path.string());
            return nullptr;
        }
    } else {
        std::istringstream ss(std::string(data.begin(), data.end()));
        roots = parse_ascii(ss);
        if (roots.empty()) {
            core::log_error("FBX", "ASCII parse failed: " + path.string());
            return nullptr;
        }
    }

    return build_mesh(roots, src_name);
}

} // namespace aether::res
