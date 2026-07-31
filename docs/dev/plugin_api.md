# Plugin API – AetherRPG Maker

**Status:** Design / Dokumentation  
**Autor:** Senior Software Architect / Lead Engine Developer  

---

## 1. Ziel

Plugins erweitern das Spiel zur Laufzeit (Runtime `Game.exe`) über Ruby (`mruby`).
Keine C++-Rekompilation nötig.

---

## 2. Manifest (`plugin.json`)

```json
{
  "name": "DemoPlugin",
  "version": "1.0.0",
  "author": "Aether Team",
  "description": "Beispiel-Plugin für EventBridge-Integration",
  "entry": "main.rb",
  "dependencies": [],
  "api_version": 1
}
```

---

## 3. Ruby-API (Engine-Module)

| Modul | Funktion | Beispiel |
|-------|----------|----------|
| `Aether::Event` | `register(name, proc)` | Ereignis im Bridge registrieren |
| `Aether::Game` | `get_state()` | GameState lesen |
| `Aether::Audio` | `bgm_play(file)` | Musik starten |

---

## 4. Plugin-Loader (C++)

`engine/src/plugin/plugin_loader.cpp` scannt `plugins/` nach `plugin.json`, lädt `entry` über `ruby::RubyVM`.

---

## 5. Demo-Plugin

Siehe: `plugins/demo_plugin/`
