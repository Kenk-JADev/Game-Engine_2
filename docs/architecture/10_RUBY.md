# Modul 10 – Ruby-Einbindung

**Namespace:** `aether::ruby`  
**Status:** implementiert (Stub-VM + API-Bootstrap; mruby später)

---

## 1. Ziel

Ruby **nur** für Spiellogik. Die Engine stellt Module bereit:

`Graphics` · `Audio` · `Input` · `SceneManager` · `Player` · `NPC` ·  
`Enemy` · `Camera` · `Weather` · `Inventory` · `Quest` · `Dialogue` · `Map`

Konzeptionell an klassischen RPG-Script-Layern orientiert, **vollständig neu**.

---

## 2. Architektur

```
RubyVM
├── StubRubyVM   # Dev/CI – Mini-Eval + Host-Dispatch
└── MRubyVm      # geplant (AETHER_WITH_MRUBY)
```

| API | Zweck |
|-----|-------|
| `define_engine_api()` | Built-in-Module + Host-Stubs |
| `define_function()` | C++-Callback als Module.method |
| `eval` / `load_file` | Script ausführen |
| `reload_file` | Hot Reload |
| `set_global` / `get_global` | `$vars` |

---

## 3. Beispiel

```ruby
Graphics.frame_rate = 60
Audio.bgm_play("Theme1", 80, 100)
Player.transfer(1, 5.0, 0.0, 5.0, 2)
Quest.start("main_001")
```

---

## 4. Tests

`tests/unit/ruby_test.cpp`

---

*Nächstes Modul: Editor / Runtime*
