/**
 * @file resource_manager.cpp
 */
#include <aether/res/resource_manager.hpp>
#include <aether/core/logger.hpp>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <sstream>

#if defined(AETHER_WITH_STB)
#  include <stb_image.h>
#endif

namespace aether::res {
namespace {

std::string normalize_key(std::string_view key) {
    std::string s(key);
    std::replace(s.begin(), s.end(), '\\', '/');
    // remove leading ./
    while (s.starts_with("./")) {
        s.erase(0, 2);
    }
    return s;
}

/** Cache-Schlüssel = Typ + logischer Pfad (gleiche Datei, verschiedene Views). */
std::string cache_key(ResourceType type, std::string_view logical) {
    return std::string(to_string(type)) + ":" + normalize_key(logical);
}

} // namespace

const char* to_string(ResourceType t) noexcept {
    switch (t) {
    case ResourceType::Unknown: return "unknown";
    case ResourceType::Text:    return "text";
    case ResourceType::Bytes:   return "bytes";
    case ResourceType::Json:    return "json";
    case ResourceType::Texture: return "texture";
    case ResourceType::Mesh:    return "mesh";
    case ResourceType::Audio:   return "audio";
    case ResourceType::Shader:  return "shader";
    }
    return "unknown";
}

ResourceManager::ResourceManager(core::ThreadPool* pool) : pool_(pool) {
    // Default-Mount: aktuelles Verzeichnis unter ""
    mounts_.push_back(VfsMount{"", fs::current_path()});
}

ResourceManager::~ResourceManager() {
    clear();
}

void ResourceManager::mount(std::string prefix, fs::path root) {
    prefix = normalize_key(prefix);
    // trailing slash entfernen
    while (!prefix.empty() && prefix.back() == '/') {
        prefix.pop_back();
    }
    std::lock_guard lock(mutex_);
    for (auto& m : mounts_) {
        if (m.prefix == prefix) {
            m.root = std::move(root);
            return;
        }
    }
    mounts_.push_back(VfsMount{std::move(prefix), std::move(root)});
    core::log_info("Res", "Mounted '" + mounts_.back().prefix + "' -> " +
                              mounts_.back().root.string());
}

void ResourceManager::unmount(std::string_view prefix) {
    const std::string p = normalize_key(prefix);
    std::lock_guard lock(mutex_);
    mounts_.erase(std::remove_if(mounts_.begin(), mounts_.end(),
                                 [&](const VfsMount& m) { return m.prefix == p; }),
                  mounts_.end());
}

fs::path ResourceManager::resolve(std::string_view logical_path) const {
    const std::string key = normalize_key(logical_path);
    std::lock_guard lock(mutex_);

    // Längste Präfix-Übereinstimmung
    const VfsMount* best = nullptr;
    usize best_len = 0;
    for (const auto& m : mounts_) {
        if (m.prefix.empty()) {
            if (!best) {
                best = &m;
            }
            continue;
        }
        if (key == m.prefix || key.starts_with(m.prefix + "/")) {
            if (m.prefix.size() >= best_len) {
                best = &m;
                best_len = m.prefix.size();
            }
        }
    }
    if (!best) {
        return {};
    }

    fs::path rel = key;
    if (!best->prefix.empty()) {
        if (key.size() == best->prefix.size()) {
            rel = "";
        } else {
            rel = key.substr(best->prefix.size() + 1);
        }
    }
    const fs::path full = best->root / rel;
    std::error_code ec;
    if (fs::exists(full, ec)) {
        return full;
    }
    return full; // auch wenn nicht existiert – Caller prüft
}

bool ResourceManager::exists(std::string_view logical_path) const {
    const fs::path p = resolve(logical_path);
    std::error_code ec;
    return !p.empty() && fs::exists(p, ec) && fs::is_regular_file(p, ec);
}

ResourceHandle ResourceManager::next_handle_unlocked() {
    return next_handle_++;
}

ResourceManager::Entry* ResourceManager::find_entry_unlocked(std::string_view key) {
    const auto it = by_key_.find(std::string(key));
    return it == by_key_.end() ? nullptr : &it->second;
}

const ResourceManager::Entry* ResourceManager::find_entry_unlocked(
    std::string_view key) const {
    const auto it = by_key_.find(std::string(key));
    return it == by_key_.end() ? nullptr : &it->second;
}

template <typename T>
Result<std::shared_ptr<T>> ResourceManager::load_or_get(
    std::string_view key_sv,
    ResourceType type,
    const std::function<Result<std::shared_ptr<T>>(const fs::path&)>& loader) {
    const std::string logical = normalize_key(key_sv);
    const std::string key = cache_key(type, logical);

    {
        std::lock_guard lock(mutex_);
        if (Entry* e = find_entry_unlocked(key)) {
            ++e->info.ref_count;
            return Result<std::shared_ptr<T>>::ok(
                std::static_pointer_cast<T>(e->data));
        }
    }

    const fs::path path = resolve(logical);
    std::error_code ec;
    if (path.empty() || !fs::exists(path, ec)) {
        return Result<std::shared_ptr<T>>::fail("Resource not found: " + logical);
    }

    auto loaded = loader(path);
    if (!loaded) {
        return loaded;
    }

    std::lock_guard lock(mutex_);
    // Double-checked: parallel load
    if (Entry* e = find_entry_unlocked(key)) {
        ++e->info.ref_count;
        return Result<std::shared_ptr<T>>::ok(std::static_pointer_cast<T>(e->data));
    }

    Entry entry;
    entry.info.handle = next_handle_unlocked();
    entry.info.type = type;
    entry.info.key = logical; // logischer Pfad für info_by_key
    entry.info.resolved_path = path;
    entry.info.byte_size = fs::file_size(path, ec);
    entry.info.ref_count = 1;
    entry.info.loaded = true;
    entry.data = std::static_pointer_cast<void>(loaded.value());

    key_by_handle_[entry.info.handle] = key;
    by_key_.emplace(key, std::move(entry));

    core::log_debug("Res", std::string("Loaded ") + to_string(type) + " '" + logical + "'");
    return loaded;
}

// explicit instantiations not needed – used only inside cpp via concrete calls

Result<std::shared_ptr<TextData>> ResourceManager::read_text_file(const fs::path& path) {
    std::ifstream in(path, std::ios::in);
    if (!in) {
        return Result<std::shared_ptr<TextData>>::fail("Cannot open: " + path.string());
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    auto data = std::make_shared<TextData>();
    data->text = ss.str();
    data->name = path.filename().string();
    return Result<std::shared_ptr<TextData>>::ok(std::move(data));
}

Result<std::shared_ptr<ByteBlob>> ResourceManager::read_bytes_file(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return Result<std::shared_ptr<ByteBlob>>::fail("Cannot open: " + path.string());
    }
    auto data = std::make_shared<ByteBlob>();
    data->name = path.filename().string();
    data->data.assign(std::istreambuf_iterator<char>(in),
                      std::istreambuf_iterator<char>());
    return Result<std::shared_ptr<ByteBlob>>::ok(std::move(data));
}

Result<std::shared_ptr<TextureData>> ResourceManager::read_texture_stub(const fs::path& path) {
#if defined(AETHER_WITH_STB)
    int w = 0, h = 0, ch = 0;
    stbi_uc* pixels = stbi_load(path.string().c_str(), &w, &h, &ch, 4);
    if (pixels) {
        auto tex = std::make_shared<TextureData>();
        tex->name = path.filename().string();
        tex->width = w;
        tex->height = h;
        tex->channels = 4;
        const usize nbytes = static_cast<usize>(w) * static_cast<usize>(h) * 4u;
        tex->pixels.assign(pixels, pixels + nbytes);
        stbi_image_free(pixels);
        core::log_debug("Res", "Texture loaded " + path.string() + " " +
                                   std::to_string(w) + "x" + std::to_string(h));
        return Result<std::shared_ptr<TextureData>>::ok(std::move(tex));
    }
    core::log_warn("Res", std::string("stb_image failed for ") + path.string() +
                              ": " + (stbi_failure_reason() ? stbi_failure_reason() : "?"));
#endif
    // Fallback-Checker
    auto tex = std::make_shared<TextureData>();
    tex->name = path.string();
    tex->width = 2;
    tex->height = 2;
    tex->channels = 4;
    tex->pixels = {
        255, 0, 255, 255,  0, 0, 0, 255,
        0, 0, 0, 255,      255, 0, 255, 255,
    };
    return Result<std::shared_ptr<TextureData>>::ok(std::move(tex));
}

Result<std::shared_ptr<render::Mesh>> ResourceManager::read_mesh_stub(const fs::path& path) {
    const auto ext = path.extension().string();
    std::string e = ext;
    std::transform(e.begin(), e.end(), e.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (e == ".obj") {
        std::ifstream in(path);
        if (in) {
            std::vector<render::Vec3> positions;
            std::vector<render::Vec3> normals;
            std::vector<render::Vec2> uvs;
            render::MeshLod lod;
            lod.max_distance = 1.0e9f;

            std::string line;
            while (std::getline(in, line)) {
                if (line.starts_with("v ")) {
                    std::istringstream ss(line.substr(2));
                    render::Vec3 p;
                    ss >> p.x >> p.y >> p.z;
                    positions.push_back(p);
                } else if (line.starts_with("vn ")) {
                    std::istringstream ss(line.substr(3));
                    render::Vec3 n;
                    ss >> n.x >> n.y >> n.z;
                    normals.push_back(n);
                } else if (line.starts_with("vt ")) {
                    std::istringstream ss(line.substr(3));
                    render::Vec2 t;
                    ss >> t.x >> t.y;
                    uvs.push_back(t);
                } else if (line.starts_with("f ")) {
                    // triangulate fan: v, v/t, v//n, v/t/n
                    std::istringstream ss(line.substr(2));
                    std::string tok;
                    std::vector<u32> face_indices;
                    while (ss >> tok) {
                        int vi = 0, ti = 0, ni = 0;
                        if (std::sscanf(tok.c_str(), "%d/%d/%d", &vi, &ti, &ni) == 3 ||
                            std::sscanf(tok.c_str(), "%d//%d", &vi, &ni) == 2 ||
                            std::sscanf(tok.c_str(), "%d/%d", &vi, &ti) == 2 ||
                            std::sscanf(tok.c_str(), "%d", &vi) == 1) {
                            render::Vertex v;
                            if (vi < 0) vi = static_cast<int>(positions.size()) + vi + 1;
                            if (vi >= 1 && vi <= static_cast<int>(positions.size())) {
                                v.position = positions[static_cast<usize>(vi - 1)];
                            }
                            if (ti < 0) ti = static_cast<int>(uvs.size()) + ti + 1;
                            if (ti >= 1 && ti <= static_cast<int>(uvs.size())) {
                                v.uv = uvs[static_cast<usize>(ti - 1)];
                            }
                            if (ni < 0) ni = static_cast<int>(normals.size()) + ni + 1;
                            if (ni >= 1 && ni <= static_cast<int>(normals.size())) {
                                v.normal = normals[static_cast<usize>(ni - 1)];
                            } else {
                                v.normal = render::Vec3(0, 1, 0);
                            }
                            v.color = render::Vec4(1.0f);
                            const u32 idx = static_cast<u32>(lod.vertices.size());
                            lod.vertices.push_back(v);
                            face_indices.push_back(idx);
                        }
                    }
                    for (usize i = 1; i + 1 < face_indices.size(); ++i) {
                        lod.indices.push_back(face_indices[0]);
                        lod.indices.push_back(face_indices[i]);
                        lod.indices.push_back(face_indices[i + 1]);
                    }
                }
            }
            if (!lod.vertices.empty() && !lod.indices.empty()) {
                // flat normals if missing
                if (normals.empty()) {
                    for (usize i = 0; i + 2 < lod.indices.size(); i += 3) {
                        auto& v0 = lod.vertices[lod.indices[i]];
                        auto& v1 = lod.vertices[lod.indices[i + 1]];
                        auto& v2 = lod.vertices[lod.indices[i + 2]];
                        const render::Vec3 n =
                            glm::normalize(glm::cross(v1.position - v0.position,
                                                      v2.position - v0.position));
                        v0.normal = v1.normal = v2.normal = n;
                    }
                }
                auto mesh = std::make_shared<render::Mesh>(path.filename().string());
                mesh->set_lod(0, std::move(lod));
                core::log_info("Res", "OBJ loaded " + path.string());
                return Result<std::shared_ptr<render::Mesh>>::ok(std::move(mesh));
            }
        }
    }

    // Fallback cube for unknown / glTF / FBX until assimp is wired
    auto mesh = render::Mesh::create_cube(1.0f);
    mesh->set_name(path.filename().string());
    core::log_debug("Res", "Mesh fallback (cube) for " + path.string());
    return Result<std::shared_ptr<render::Mesh>>::ok(std::move(mesh));
}

Result<std::shared_ptr<TextData>> ResourceManager::load_text(std::string_view key) {
    return load_or_get<TextData>(key, ResourceType::Text, read_text_file);
}

Result<std::shared_ptr<ByteBlob>> ResourceManager::load_bytes(std::string_view key) {
    return load_or_get<ByteBlob>(key, ResourceType::Bytes, read_bytes_file);
}

Result<std::shared_ptr<nlohmann::json>> ResourceManager::load_json(std::string_view key) {
    return load_or_get<nlohmann::json>(
        key, ResourceType::Json, [](const fs::path& path) -> Result<std::shared_ptr<nlohmann::json>> {
            std::ifstream in(path);
            if (!in) {
                return Result<std::shared_ptr<nlohmann::json>>::fail("Cannot open: " +
                                                                     path.string());
            }
            try {
                auto j = std::make_shared<nlohmann::json>();
                in >> *j;
                return Result<std::shared_ptr<nlohmann::json>>::ok(std::move(j));
            } catch (const std::exception& ex) {
                return Result<std::shared_ptr<nlohmann::json>>::fail(
                    std::string("JSON error: ") + ex.what());
            }
        });
}

Result<std::shared_ptr<TextureData>> ResourceManager::load_texture(std::string_view key) {
    return load_or_get<TextureData>(key, ResourceType::Texture, read_texture_stub);
}

Result<std::shared_ptr<render::Mesh>> ResourceManager::load_mesh(std::string_view key) {
    return load_or_get<render::Mesh>(key, ResourceType::Mesh, read_mesh_stub);
}

std::future<Result<std::shared_ptr<TextData>>> ResourceManager::load_text_async(
    std::string_view key) {
    const std::string key_copy(key);
    if (!pool_) {
        std::promise<Result<std::shared_ptr<TextData>>> prom;
        prom.set_value(load_text(key_copy));
        return prom.get_future();
    }
    return pool_->submit([this, key_copy] { return load_text(key_copy); });
}

void ResourceManager::release(ResourceHandle handle) {
    std::lock_guard lock(mutex_);
    const auto it = key_by_handle_.find(handle);
    if (it == key_by_handle_.end()) {
        return;
    }
    auto eit = by_key_.find(it->second);
    if (eit == by_key_.end()) {
        return;
    }
    if (eit->second.info.ref_count > 0) {
        --eit->second.info.ref_count;
    }
}

usize ResourceManager::unload_unused() {
    std::lock_guard lock(mutex_);
    usize removed = 0;
    for (auto it = by_key_.begin(); it != by_key_.end();) {
        if (it->second.info.ref_count == 0) {
            key_by_handle_.erase(it->second.info.handle);
            it = by_key_.erase(it);
            ++removed;
        } else {
            ++it;
        }
    }
    return removed;
}

void ResourceManager::clear() {
    std::lock_guard lock(mutex_);
    by_key_.clear();
    key_by_handle_.clear();
    fallback_tex_.reset();
}

usize ResourceManager::cached_count() const {
    std::lock_guard lock(mutex_);
    return by_key_.size();
}

usize ResourceManager::cached_bytes() const {
    std::lock_guard lock(mutex_);
    usize total = 0;
    for (const auto& [k, e] : by_key_) {
        (void)k;
        total += e.info.byte_size;
    }
    return total;
}

std::optional<ResourceInfo> ResourceManager::info(ResourceHandle handle) const {
    std::lock_guard lock(mutex_);
    const auto it = key_by_handle_.find(handle);
    if (it == key_by_handle_.end()) {
        return std::nullopt;
    }
    const auto eit = by_key_.find(it->second);
    if (eit == by_key_.end()) {
        return std::nullopt;
    }
    return eit->second.info;
}

std::optional<ResourceInfo> ResourceManager::info_by_key(std::string_view key) const {
    std::lock_guard lock(mutex_);
    const std::string logical = normalize_key(key);
    // Suche über alle Typ-Präfixe
    for (const auto& [k, e] : by_key_) {
        if (e.info.key == logical) {
            return e.info;
        }
        (void)k;
    }
    return std::nullopt;
}

std::shared_ptr<TextureData> ResourceManager::fallback_texture() {
    std::lock_guard lock(mutex_);
    if (!fallback_tex_) {
        fallback_tex_ = std::make_shared<TextureData>();
        fallback_tex_->name = "<fallback>";
        fallback_tex_->width = 1;
        fallback_tex_->height = 1;
        fallback_tex_->channels = 4;
        fallback_tex_->pixels = {255, 0, 255, 255};
    }
    return fallback_tex_;
}

} // namespace aether::res
