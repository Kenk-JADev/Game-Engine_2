# Modul 12 – Runtime (Game.exe)

**Target:** `Game`  
**Status:** implementiert (Headless + GLFW/OpenGL; RubyHost-Anfragen, Transfer, Szenen)

---

## Ablauf

```
parse_args
 → load project.json
 → EngineContext::create(RuntimeConfig)
 → ResourceManager mounts
 → Window / Renderer / Input / Audio
 → RubyVM::define_engine_api + load scripts/main.rb
 → main loop: poll → update → fixed → render → swap
 → shutdown
```

## CLI

```bash
Game --project samples/demo_project --headless --max-frames 5
Game -p /path/MyGame
Game --help
```

## Prinzipien

- Kein Editor-Code in der Runtime
- Lädt Projekt, Assets, Scripts, Plugins automatisch
- Startet direkt ins Spiel

---

*Plugin-Loader und volles Eventsystem folgen.*
