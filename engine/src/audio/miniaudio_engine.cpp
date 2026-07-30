/**
 * @file miniaudio_engine.cpp
 */
#include "miniaudio_engine.hpp"

#if defined(AETHER_WITH_MINIAUDIO)

#include <aether/core/logger.hpp>

#include <miniaudio.h>

#include <algorithm>
#include <cstring>
#include <memory>

namespace aether::audio {

MiniAudioEngine::MiniAudioEngine(core::AudioConfig config)
    : AudioEngine(AudioBackend::MiniAudio, std::move(config)) {
    ready_ = init();
    if (ready_) {
        core::log_info("Audio", "MiniAudioEngine ready");
    } else {
        core::log_error("Audio", "MiniAudioEngine init failed");
    }
}

MiniAudioEngine::~MiniAudioEngine() {
    shutdown();
}

bool MiniAudioEngine::init() {
    engine_ = new ma_engine();
    std::memset(engine_, 0, sizeof(ma_engine));
    const ma_result r = ma_engine_init(nullptr, engine_);
    if (r != MA_SUCCESS) {
        delete engine_;
        engine_ = nullptr;
        return false;
    }
    return true;
}

void MiniAudioEngine::shutdown() {
    backend_stop(AudioChannel::Bgm, 0);
    backend_stop(AudioChannel::Bgs, 0);
    backend_stop(AudioChannel::Me, 0);
    if (engine_) {
        ma_engine_uninit(engine_);
        delete engine_;
        engine_ = nullptr;
    }
    ready_ = false;
}

ma_sound* MiniAudioEngine::play_path(const std::string& path, bool loop,
                                     const PlayParams& params) {
    if (!ready_ || !engine_) return nullptr;
    auto* sound = new ma_sound();
    std::memset(sound, 0, sizeof(ma_sound));
    ma_uint32 flags = loop ? MA_SOUND_FLAG_STREAM : MA_SOUND_FLAG_DECODE;
    if (loop) flags |= MA_SOUND_FLAG_STREAM;
    const ma_result r = ma_sound_init_from_file(engine_, path.c_str(), flags, nullptr,
                                                nullptr, sound);
    if (r != MA_SUCCESS) {
        delete sound;
        core::log_warn("Audio", "Failed to load " + path);
        return nullptr;
    }
    ma_sound_set_looping(sound, loop ? MA_TRUE : MA_FALSE);
    const float vol = static_cast<float>(std::clamp(params.volume, 0, 100)) / 100.0f;
    ma_sound_set_volume(sound, vol);
    const float pitch = static_cast<float>(std::clamp(params.pitch, 50, 150)) / 100.0f;
    ma_sound_set_pitch(sound, pitch);
    if (params.pos > 0.0) {
        ma_sound_seek_to_pcm_frame(sound, 0); // simplified
    }
    ma_sound_start(sound);
    return sound;
}

void MiniAudioEngine::backend_play(AudioChannel channel, std::string_view name,
                                   const PlayParams& params) {
    if (!ready_) return;
    const std::string path = resolve_path(name);

    auto stop_sound = [](ma_sound*& s) {
        if (s) {
            ma_sound_uninit(s);
            delete s;
            s = nullptr;
        }
    };

    switch (channel) {
    case AudioChannel::Bgm:
        stop_sound(bgm_sound_);
        bgm_sound_ = play_path(path, true, params);
        break;
    case AudioChannel::Bgs:
        stop_sound(bgs_sound_);
        bgs_sound_ = play_path(path, true, params);
        break;
    case AudioChannel::Me:
        stop_sound(me_sound_);
        me_sound_ = play_path(path, false, params);
        break;
    case AudioChannel::Se:
        // fire-and-forget: engine manages short sounds via ma_engine_play_sound
        if (engine_) {
            const float vol = static_cast<float>(std::clamp(params.volume, 0, 100)) / 100.0f;
            ma_engine_set_volume(engine_, 1.0f);
            ma_engine_play_sound(engine_, path.c_str(), nullptr);
            (void)vol;
        }
        break;
    }
}

void MiniAudioEngine::backend_stop(AudioChannel channel, i32 fade_ms) {
    (void)fade_ms;
    auto stop_sound = [](ma_sound*& s) {
        if (s) {
            ma_sound_stop(s);
            ma_sound_uninit(s);
            delete s;
            s = nullptr;
        }
    };
    switch (channel) {
    case AudioChannel::Bgm: stop_sound(bgm_sound_); break;
    case AudioChannel::Bgs: stop_sound(bgs_sound_); break;
    case AudioChannel::Me:  stop_sound(me_sound_); break;
    case AudioChannel::Se:  break;
    }
}

void MiniAudioEngine::update(f64 dt) {
    AudioEngine::update(dt);
    // miniaudio engine runs on its own thread
}

} // namespace aether::audio

#endif
