/**
 * @file null_audio.cpp
 */
#include "null_audio.hpp"

#include <aether/core/logger.hpp>

#include <string>

namespace aether::audio {

NullAudioEngine::NullAudioEngine(core::AudioConfig config)
    : AudioEngine(AudioBackend::Null, std::move(config)) {
    core::log_info("Audio", "NullAudioEngine created");
}

void NullAudioEngine::backend_play(AudioChannel channel, std::string_view name,
                                   const PlayParams& params) {
    core::log_debug("Audio",
                    std::string("play ") +
                        (channel == AudioChannel::Bgm   ? "BGM "
                         : channel == AudioChannel::Bgs ? "BGS "
                         : channel == AudioChannel::Me  ? "ME "
                                                        : "SE ") +
                        std::string(name) + " vol=" + std::to_string(params.volume) +
                        " pitch=" + std::to_string(params.pitch));
}

void NullAudioEngine::backend_stop(AudioChannel channel, i32 fade_ms) {
    (void)channel;
    (void)fade_ms;
}

} // namespace aether::audio
