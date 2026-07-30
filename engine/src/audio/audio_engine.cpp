/**
 * @file audio_engine.cpp
 */
#include <aether/audio/audio_engine.hpp>

#include "null_audio.hpp"

#if defined(AETHER_WITH_MINIAUDIO)
#  include "miniaudio_engine.hpp"
#endif

#include <aether/core/logger.hpp>

#include <algorithm>

namespace aether::audio {
namespace {

i32 clamp_vol(i32 v) {
    return std::clamp(v, 0, 100);
}

} // namespace

AudioEngine::AudioEngine(AudioBackend backend, core::AudioConfig config)
    : backend_(backend)
    , config_(std::move(config)) {}

AudioEngine::~AudioEngine() = default;

std::unique_ptr<AudioEngine> AudioEngine::create(AudioBackend backend,
                                                 const core::AudioConfig& config) {
    switch (backend) {
    case AudioBackend::MiniAudio:
#if defined(AETHER_WITH_MINIAUDIO)
    {
        auto eng = std::make_unique<MiniAudioEngine>(config);
        return eng;
    }
#else
        core::log_warn("Audio", "MiniAudio not enabled at build time – using Null");
        [[fallthrough]];
#endif
    case AudioBackend::Null:
    default:
        return std::make_unique<NullAudioEngine>(config);
    }
}

void AudioEngine::set_master_volume(AudioChannel channel, i32 volume_0_100) {
    const i32 v = clamp_vol(volume_0_100);
    switch (channel) {
    case AudioChannel::Bgm: config_.bgm_volume = v; break;
    case AudioChannel::Bgs: config_.bgs_volume = v; break;
    case AudioChannel::Me:  config_.me_volume  = v; break;
    case AudioChannel::Se:  config_.se_volume  = v; break;
    }
}

i32 AudioEngine::master_volume(AudioChannel channel) const noexcept {
    switch (channel) {
    case AudioChannel::Bgm: return config_.bgm_volume;
    case AudioChannel::Bgs: return config_.bgs_volume;
    case AudioChannel::Me:  return config_.me_volume;
    case AudioChannel::Se:  return config_.se_volume;
    }
    return 0;
}

void AudioEngine::register_clip(std::string name, std::string path) {
    clips_[std::move(name)] = std::move(path);
}

bool AudioEngine::has_clip(std::string_view name) const {
    return clips_.find(std::string(name)) != clips_.end();
}

std::string AudioEngine::resolve_path(std::string_view name) const {
    const auto it = clips_.find(std::string(name));
    if (it != clips_.end()) {
        return it->second;
    }
    return std::string(name);
}

void AudioEngine::bgm_play(std::string_view name, const PlayParams& params) {
    bgm_.playing = true;
    bgm_.name = std::string(name);
    bgm_.params = params;
    bgm_.params.volume = clamp_vol(params.volume);
    bgm_.playback_seconds = params.pos;
    ++stats_.bgm_play_count;
    backend_play(AudioChannel::Bgm, name, bgm_.params);
}

void AudioEngine::bgm_stop(i32 fade_ms) {
    if (!bgm_.playing && fade_ms <= 0) {
        return;
    }
    backend_stop(AudioChannel::Bgm, fade_ms);
    if (fade_ms <= 0) {
        bgm_ = {};
    }
    ++stats_.stop_count;
}

void AudioEngine::bgm_fade(i32 target_volume, i32 fade_ms) {
    if (fade_ms <= 0) {
        bgm_.params.volume = clamp_vol(target_volume);
        return;
    }
    fade_.active = true;
    fade_.channel = AudioChannel::Bgm;
    fade_.from = static_cast<f32>(bgm_.params.volume);
    fade_.to = static_cast<f32>(clamp_vol(target_volume));
    fade_.elapsed = 0.0f;
    fade_.duration = static_cast<f32>(fade_ms) / 1000.0f;
}

void AudioEngine::bgs_play(std::string_view name, const PlayParams& params) {
    bgs_.playing = true;
    bgs_.name = std::string(name);
    bgs_.params = params;
    bgs_.params.volume = clamp_vol(params.volume);
    ++stats_.bgs_play_count;
    backend_play(AudioChannel::Bgs, name, bgs_.params);
}

void AudioEngine::bgs_stop(i32 fade_ms) {
    backend_stop(AudioChannel::Bgs, fade_ms);
    if (fade_ms <= 0) {
        bgs_ = {};
    }
    ++stats_.stop_count;
}

void AudioEngine::me_play(std::string_view name, const PlayParams& params) {
    me_.playing = true;
    me_.name = std::string(name);
    me_.params = params;
    me_.params.volume = clamp_vol(params.volume);
    ++stats_.me_play_count;
    backend_play(AudioChannel::Me, name, me_.params);
}

void AudioEngine::me_stop(i32 fade_ms) {
    backend_stop(AudioChannel::Me, fade_ms);
    if (fade_ms <= 0) {
        me_ = {};
    }
    ++stats_.stop_count;
}

void AudioEngine::se_play(std::string_view name, const PlayParams& params) {
    PlayParams p = params;
    p.volume = clamp_vol(params.volume);
    ++stats_.se_play_count;
    backend_play(AudioChannel::Se, name, p);
}

void AudioEngine::se_stop() {
    backend_stop(AudioChannel::Se, 0);
    ++stats_.stop_count;
}

void AudioEngine::stop_all() {
    bgm_stop(0);
    bgs_stop(0);
    me_stop(0);
    se_stop();
}

void AudioEngine::update(f64 dt) {
    if (bgm_.playing) {
        bgm_.playback_seconds += dt;
    }
    if (bgs_.playing) {
        bgs_.playback_seconds += dt;
    }
    if (me_.playing) {
        me_.playback_seconds += dt;
    }

    if (fade_.active) {
        fade_.elapsed += static_cast<f32>(dt);
        const f32 t = fade_.duration > 0.0f
                          ? std::clamp(fade_.elapsed / fade_.duration, 0.0f, 1.0f)
                          : 1.0f;
        const f32 vol = fade_.from + (fade_.to - fade_.from) * t;
        if (fade_.channel == AudioChannel::Bgm) {
            bgm_.params.volume = static_cast<i32>(vol);
        }
        if (t >= 1.0f) {
            fade_.active = false;
            if (fade_.to <= 0.0f && fade_.channel == AudioChannel::Bgm) {
                bgm_ = {};
            }
        }
    }
}

} // namespace aether::audio
