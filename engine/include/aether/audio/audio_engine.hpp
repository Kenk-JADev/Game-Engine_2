/**
 * @file audio_engine.hpp
 * @brief Audio-Engine mit BGM/BGS/ME/SE-Kanälen (RPG-Maker-Stil).
 *
 * Backend:
 *  - NullAudio  (immer verfügbar, loggt / zählt Aufrufe)
 *  - MiniAudio  (später, wenn AETHER_WITH_MINIAUDIO)
 *
 * Ruby-API-Ziel:
 *   Audio.bgm_play("Theme1", volume, pitch)
 *   Audio.se_play("Open1", volume, pitch)
 */
#pragma once

#include <aether/core/config.hpp>
#include <aether/core/types.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace aether::audio {

enum class AudioBackend {
    Null,
    MiniAudio,
};

/** @brief Kanaltypen analog klassischer RPG Maker. */
enum class AudioChannel {
    Bgm, ///< Background Music (loop)
    Bgs, ///< Background Sound (loop, ambient)
    Me,  ///< Music Effect (fanfare, one-shot, pausiert oft BGM)
    Se,  ///< Sound Effect (one-shot, viele parallel)
};

struct AudioClipDesc {
    std::string path;
    std::string name; ///< logischer Name (ohne Pfad/Extension)
};

struct PlayParams {
    i32 volume = 80; ///< 0–100
    i32 pitch  = 100; ///< 50–150 typisch
    f64 pos    = 0.0; ///< Startposition in Sekunden (BGM)
};

struct ChannelState {
    bool playing = false;
    std::string name;
    PlayParams params{};
    f64 playback_seconds = 0.0;
};

/**
 * @brief Audio-Statistik (Tests / Debug).
 */
struct AudioStats {
    u32 bgm_play_count = 0;
    u32 bgs_play_count = 0;
    u32 me_play_count  = 0;
    u32 se_play_count  = 0;
    u32 stop_count     = 0;
};

/**
 * @brief Öffentliche Audio-API.
 */
class AudioEngine : public aether::NonMovable {
public:
    [[nodiscard]] static std::unique_ptr<AudioEngine> create(AudioBackend backend,
                                                             const core::AudioConfig& config);

    virtual ~AudioEngine();

    [[nodiscard]] AudioBackend backend() const noexcept { return backend_; }
    [[nodiscard]] const AudioStats& stats() const noexcept { return stats_; }

    void set_master_volume(AudioChannel channel, i32 volume_0_100);
    [[nodiscard]] i32 master_volume(AudioChannel channel) const noexcept;

    /**
     * @brief Registriert einen Clip-Namen → Dateipfad.
     */
    void register_clip(std::string name, std::string path);

    /**
     * @brief Spielt BGM (loop). Stoppt vorheriges BGM.
     */
    void bgm_play(std::string_view name, const PlayParams& params = {});
    void bgm_stop(i32 fade_ms = 0);
    void bgm_fade(i32 target_volume, i32 fade_ms);

    void bgs_play(std::string_view name, const PlayParams& params = {});
    void bgs_stop(i32 fade_ms = 0);

    void me_play(std::string_view name, const PlayParams& params = {});
    void me_stop(i32 fade_ms = 0);

    void se_play(std::string_view name, const PlayParams& params = {});
    void se_stop();

    /** @brief Stoppt alle Kanäle. */
    void stop_all();

    /**
     * @brief Tick für Fades / virtuelle Playback-Zeit (Null-Backend).
     * @param dt Delta in Sekunden
     */
    virtual void update(f64 dt);

    [[nodiscard]] const ChannelState& bgm_state() const noexcept { return bgm_; }
    [[nodiscard]] const ChannelState& bgs_state() const noexcept { return bgs_; }
    [[nodiscard]] const ChannelState& me_state() const noexcept { return me_; }

    [[nodiscard]] bool has_clip(std::string_view name) const;

protected:
    AudioEngine(AudioBackend backend, core::AudioConfig config);

    virtual void backend_play(AudioChannel channel, std::string_view name,
                              const PlayParams& params) = 0;
    virtual void backend_stop(AudioChannel channel, i32 fade_ms) = 0;

    [[nodiscard]] std::string resolve_path(std::string_view name) const;

    AudioBackend backend_;
    core::AudioConfig config_;
    AudioStats stats_{};

    ChannelState bgm_{};
    ChannelState bgs_{};
    ChannelState me_{};

    std::unordered_map<std::string, std::string> clips_;

    struct Fade {
        bool active = false;
        AudioChannel channel = AudioChannel::Bgm;
        f32 from = 0;
        f32 to = 0;
        f32 elapsed = 0;
        f32 duration = 0;
    };
    Fade fade_{};
};

} // namespace aether::audio
