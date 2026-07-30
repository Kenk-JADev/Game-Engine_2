/**
 * @file miniaudio_engine.hpp
 */
#pragma once

#include <aether/audio/audio_engine.hpp>

#if defined(AETHER_WITH_MINIAUDIO)

struct ma_engine;
struct ma_sound;

namespace aether::audio {

class MiniAudioEngine final : public AudioEngine {
public:
    explicit MiniAudioEngine(core::AudioConfig config);
    ~MiniAudioEngine() override;

    void update(f64 dt) override;

protected:
    void backend_play(AudioChannel channel, std::string_view name,
                      const PlayParams& params) override;
    void backend_stop(AudioChannel channel, i32 fade_ms) override;

private:
    bool init();
    void shutdown();
    ma_sound* play_path(const std::string& path, bool loop, const PlayParams& params);

    ma_engine* engine_ = nullptr;
    ma_sound* bgm_sound_ = nullptr;
    ma_sound* bgs_sound_ = nullptr;
    ma_sound* me_sound_ = nullptr;
    bool ready_ = false;
};

} // namespace aether::audio

#endif
