/**
 * @file null_audio.hpp
 * @brief Audio-Backend ohne Hardware (Tests / Headless).
 */
#pragma once

#include <aether/audio/audio_engine.hpp>

namespace aether::audio {

class NullAudioEngine final : public AudioEngine {
public:
    explicit NullAudioEngine(core::AudioConfig config);

protected:
    void backend_play(AudioChannel channel, std::string_view name,
                      const PlayParams& params) override;
    void backend_stop(AudioChannel channel, i32 fade_ms) override;
};

} // namespace aether::audio
