# Modul 9 – Ressourcenverwaltung

**Namespace:** `aether::res`  
**Status:** implementiert (Phase-1-Loader; stb/assimp später)

---

## 1. Architektur

```
ResourceManager
├── VFS Mounts (prefix → directory)
├── Typed Cache (text, bytes, json, texture, mesh)
├── Ref-Counting + unload_unused()
└── Async Load (ThreadPool)
```

| API | Beschreibung |
|-----|--------------|
| `mount(prefix, root)` | Logischer Pfad-Präfix |
| `resolve` / `exists` | Dateisuche |
| `load_text` / `load_bytes` / `load_json` | Synchron + Cache |
| `load_texture` / `load_mesh` | Stub-Loader (Phase 1) |
| `load_text_async` | Hintergrund via ThreadPool |
| `release` / `unload_unused` / `clear` | Speicherpflege |

---

## 2. Projekt-Mounts (Runtime-Konvention)

```
res.mount("data",     project / "data");
res.mount("maps",     project / "maps");
res.mount("graphics", project / "graphics");
res.mount("audio",    project / "audio");
res.mount("scripts",  project / "scripts");
```

Logische Keys entsprechen RPG-Maker-artigen Pfaden:

```
graphics/characters/Hero.png
audio/bgm/Theme1.ogg
data/actors.json
```

---

## 3. Formate

| Typ | Geplant | Phase 1 |
|-----|---------|---------|
| JSON | nlohmann | ✅ voll |
| Text/Bytes | iostream | ✅ voll |
| PNG/JPG | stb_image | Stub (Checkerboard, Datei muss existieren) |
| glTF/FBX/OBJ | assimp | Stub (Cube-Fallback) |
| WAV/OGG/MP3 | miniaudio | über Audio-Clip-Registry |

---

## 4. Nutzung

```cpp
ResourceManager res(&ctx->thread_pool());
res.mount("data", "MyGame/data");

auto actors = res.load_json("data/actors.json");
if (actors) {
    // (*actors.value())["…"]
}
```

---

## 5. Tests

`tests/unit/res_test.cpp`

---

*Nächstes Modul: Ruby-Einbindung*
