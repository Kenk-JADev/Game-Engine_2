# Modul 8 – Audio

**Namespace:** `aether::audio`  
**Status:** implementiert (Null-Backend; MiniAudio später)

---

## 1. Kanäle (RPG-Maker-Stil)

| Kanal | Bedeutung | Loop |
|-------|-----------|------|
| **BGM** | Background Music | ja |
| **BGS** | Ambient / Background Sound | ja |
| **ME** | Fanfare / Music Effect | nein |
| **SE** | Sound Effects (parallel) | nein |

Ruby-Ziel-API:

```ruby
Audio.bgm_play("Theme1", 80, 100)
Audio.bgs_play("Rain", 40, 100)
Audio.me_play("Victory", 80, 100)
Audio.se_play("Open1", 80, 100)
Audio.bgm_stop(500) # fade ms
```

---

## 2. Architektur

```
AudioEngine (API)
└── NullAudioEngine   # Headless / Tests
# └── MiniAudioEngine # geplant
```

Features Phase 1:

- Clip-Registry (`name → path`)
- Play/Stop pro Kanal
- Master-Volumes (0–100)
- BGM-Fade
- Virtuelle Playback-Zeit (`update(dt)`)
- Stats für Tests

---

## 3. Tests

`tests/unit/audio_test.cpp`

---

*Nächstes Modul: Ressourcenverwaltung*
