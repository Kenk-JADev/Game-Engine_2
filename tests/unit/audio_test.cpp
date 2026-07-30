/**
 * @file audio_test.cpp
 */
#include <aether/audio/audio_module.hpp>
#include <aether/core/core.hpp>

#include <cstdio>

using namespace aether::core;
using namespace aether::audio;

static int g_failures = 0;
#define CHECK(cond)                                                            \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::fprintf(stderr, "CHECK failed: %s (%s:%d)\n", #cond,         \
                         __FILE__, __LINE__);                                  \
            ++g_failures;                                                      \
        }                                                                      \
    } while (0)

int main() {
    AudioConfig acfg;
    auto audio = AudioEngine::create(AudioBackend::Null, acfg);
    CHECK(audio != nullptr);

    audio->register_clip("Theme1", "audio/bgm/Theme1.ogg");
    audio->register_clip("Open1", "audio/se/Open1.wav");
    CHECK(audio->has_clip("Theme1"));

    audio->bgm_play("Theme1", PlayParams{80, 100, 0.0});
    CHECK(audio->bgm_state().playing);
    CHECK(audio->bgm_state().name == "Theme1");
    CHECK(audio->stats().bgm_play_count == 1);

    audio->update(0.5);
    CHECK(audio->bgm_state().playback_seconds >= 0.49);

    audio->se_play("Open1", PlayParams{90, 100});
    CHECK(audio->stats().se_play_count == 1);

    audio->bgs_play("Rain", PlayParams{40, 100});
    CHECK(audio->bgs_state().playing);

    audio->me_play("Victory", PlayParams{80, 100});
    CHECK(audio->me_state().playing);

    audio->bgm_fade(0, 100); // 100ms fade out
    audio->update(0.05);
    CHECK(audio->bgm_state().params.volume < 80);
    audio->update(0.1);
    // after fade to 0 should stop
    CHECK(!audio->bgm_state().playing || audio->bgm_state().params.volume == 0);

    audio->stop_all();
    CHECK(!audio->bgs_state().playing);
    CHECK(!audio->me_state().playing);

    audio->set_master_volume(AudioChannel::Se, 50);
    CHECK(audio->master_volume(AudioChannel::Se) == 50);

    if (g_failures == 0) {
        std::puts("OK: audio_test passed");
        return 0;
    }
    std::fprintf(stderr, "FAIL: %d check(s) failed\n", g_failures);
    return 1;
}
