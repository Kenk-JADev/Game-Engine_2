/**
 * @file resource_manager.hpp
 * @brief Asset-Cache, Pfadauflösung und asynchrones Laden.
 *
 * Unterstützte Formate (Master-Prompt):
 *   Modelle: glTF, GLB (cgltf), FBX (eigener Reader), OBJ
 *   Texturen: PNG, JPG (stb_image)
 *   Audio: WAV, OGG, MP3 (miniaudio / Null)
 *   Daten: JSON
 *
 * Directory-Mounts, Cache mit Referenzzählung, async via ThreadPool.
 */
#pragma once

#include <aether/core/thread_pool.hpp>
#include <aether/core/types.hpp>
#include <aether/render/mesh.hpp>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace aether::res {

namespace fs = std::filesystem;

/** @brief Art einer Ressource. */
enum class ResourceType {
    Unknown,
    Text,
    Bytes,
    Json,
    Texture,
    Mesh,
    Audio,
    Shader,
};

/** @brief Opaque Handle (0 = invalid). */
using ResourceHandle = u32;

inline constexpr ResourceHandle kInvalidResource = 0;

/**
 * @brief Metadaten eines Cache-Eintrags.
 */
struct ResourceInfo {
    ResourceHandle handle = kInvalidResource;
    ResourceType type = ResourceType::Unknown;
    std::string key; ///< logischer Schlüssel
    fs::path resolved_path;
    usize byte_size = 0;
    u32 ref_count = 0;
    bool loaded = false;
};

/** @brief CPU-Texturdaten (ohne GPU-Upload). */
struct TextureData {
    i32 width = 0;
    i32 height = 0;
    i32 channels = 0;
    std::vector<u8> pixels;
    std::string name;
};

/** @brief Rohe Byte-Blob-Ressource. */
struct ByteBlob {
    std::vector<u8> data;
    std::string name;
};

/** @brief Text-Ressource. */
struct TextData {
    std::string text;
    std::string name;
};

/** @brief Mount-Punkt: Präfix → Verzeichnis. */
struct VfsMount {
    std::string prefix;
    fs::path root;
};

/**
 * @brief Zentraler Ressourcen-Manager.
 */
class ResourceManager : public aether::NonMovable {
public:
    explicit ResourceManager(core::ThreadPool* pool = nullptr);
    ~ResourceManager();

    void set_thread_pool(core::ThreadPool* pool) noexcept { pool_ = pool; }

    /** @brief Hängt ein Verzeichnis unter einem logischen Präfix ein. */
    void mount(std::string prefix, fs::path root);

    void unmount(std::string_view prefix);

    /** @brief Sucht Datei über Mounts. */
    [[nodiscard]] fs::path resolve(std::string_view logical_path) const;

    [[nodiscard]] bool exists(std::string_view logical_path) const;

    [[nodiscard]] Result<std::shared_ptr<TextData>> load_text(std::string_view key);
    [[nodiscard]] Result<std::shared_ptr<ByteBlob>> load_bytes(std::string_view key);
    [[nodiscard]] Result<std::shared_ptr<nlohmann::json>> load_json(std::string_view key);
    [[nodiscard]] Result<std::shared_ptr<TextureData>> load_texture(std::string_view key);
    [[nodiscard]] Result<std::shared_ptr<render::Mesh>> load_mesh(std::string_view key);

    [[nodiscard]] std::future<Result<std::shared_ptr<TextData>>> load_text_async(
        std::string_view key);

    void release(ResourceHandle handle);
    usize unload_unused();
    void clear();

    [[nodiscard]] usize cached_count() const;
    [[nodiscard]] usize cached_bytes() const;

    [[nodiscard]] std::optional<ResourceInfo> info(ResourceHandle handle) const;
    [[nodiscard]] std::optional<ResourceInfo> info_by_key(std::string_view key) const;

    [[nodiscard]] std::shared_ptr<TextureData> fallback_texture();

private:
    struct Entry {
        ResourceInfo info;
        std::shared_ptr<void> data;
    };

    ResourceHandle next_handle_unlocked();
    Entry* find_entry_unlocked(std::string_view key);
    const Entry* find_entry_unlocked(std::string_view key) const;

    template <typename T>
    Result<std::shared_ptr<T>> load_or_get(
        std::string_view key,
        ResourceType type,
        const std::function<Result<std::shared_ptr<T>>(const fs::path&)>& loader);

    static Result<std::shared_ptr<TextData>> read_text_file(const fs::path& path);
    static Result<std::shared_ptr<ByteBlob>> read_bytes_file(const fs::path& path);
    static Result<std::shared_ptr<TextureData>> read_texture_stub(const fs::path& path);
    static Result<std::shared_ptr<render::Mesh>> read_mesh_stub(const fs::path& path);

    core::ThreadPool* pool_ = nullptr;
    std::vector<VfsMount> mounts_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, Entry> by_key_;
    std::unordered_map<ResourceHandle, std::string> key_by_handle_;
    ResourceHandle next_handle_ = 1;
    std::shared_ptr<TextureData> fallback_tex_;
};

[[nodiscard]] const char* to_string(ResourceType t) noexcept;

} // namespace aether::res
