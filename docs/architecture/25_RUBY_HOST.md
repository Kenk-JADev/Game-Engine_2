# Modul 25 – Ruby-Host-API (echte Engine-Bindings)

**Namespace:** `aether::ruby`  \
**Dateien:** `engine/include/aether/ruby/ruby_host.hpp`, `engine/src/ruby/ruby_host.cpp`  \
**Status:** implementiert

---

## 1. Ziel

Das Master-Prompt fordert Ruby **ausschließlich für Spiellogik** mit APIs für:

`Graphics · Audio · Input · SceneManager · Player · NPC · Enemy · Camera · Weather · Inventory · Quest · Dialogue · Map`

Bisher waren die Host-Bindings No-Op-Stubs („nil“). `RubyHost` bindet die Module jetzt an
**echte Engine-Systeme** – über die vorhandene String-Schnittstelle von `RubyVM::define_function`,
damit Stub-VM (Headless/Tests) und mruby-VM denselben Code nutzen.

## 2. Architektur

```
Ruby-Skript
   │  Module.method(args)
   ▼
RubyVM (Stub oder mruby)
   │  call_host(module, name, args)
   ▼
RubyHost (install())
   │  liest/schreibt über RubyHostBindings (nicht-besitzende Zeiger)
   ▼
GameState · PartyInventory · QuestLog · WeatherSystem · PlayerController ·
Camera · InputManager · AudioEngine · GameSceneStack · GameContext ·
Database · Scene
```

### RubyHostBindings (Auswahl)

| Feld | System |
|------|--------|
| `state` | Schalter/Variablen (`Game.switch`, `Game.set_variable`) |
| `inventory` | Gold/Items (`Inventory.gain/lose/count/gold`) |
| `quests` | `Quest.start/complete/active?/…` |
| `weather` | `Weather.set/clear/type/power` |
| `player` | `Player.x/y/z/transfer/move/…` |
| `camera` | `Camera.move_to/look_at/zoom/shake` |
| `input` | `Input.press?/trigger?/dir4/dir8` |
| `audio` | `Audio.bgm_play/se_play/…` |
| `scenes` + `gctx` | `SceneManager.goto`, `Dialogue.start/choices`, NPC.say |
| `database` + `scene` | Item-Name-Auflösung, `Enemy.spawn`, `NPC.set_position` |

Fehlende Systeme (`nullptr`) → sichere No-Ops bzw. Defaults; Skripte laufen überall.

## 3. API-Überblick

```ruby
Graphics.frame_rate = 60
Graphics.fade_out(30)          # Runtime blendet aus

Audio.bgm_play("Theme1", 80, 100)
Audio.se_play("Open1")
Audio.bgm_stop(500)

Input.press?(:C)               # confirm
Input.trigger?(:B)             # cancel
Input.dir4                     # 2/4/6/8

SceneManager.goto(:menu)
SceneManager.goto("exit")      # beendet das Spiel

Player.transfer(2, 5.0, 0.0, 5.0, 2)
Player.set_position(1, 0, 3)
Player.move(0, 0, 1)

NPC.find("Bob")                # Entity-Id oder nil
NPC.set_position("Bob", 10, 0, 12)
NPC.say("Bob", "Hallo!")

Enemy.spawn(1, 0, 0, 0)
Enemy.count
Enemy.kill("Enemy_1")

Camera.move_to(0, 8, 10)
Camera.zoom(60)
Camera.shake(3, 0.5)

Weather.set("rain", 7)
Weather.clear

Inventory.gain(1, 3)           # Item-Id
Inventory.gain("Potion", 2)    # oder Name aus der Datenbank
Inventory.gold = 100
Inventory.has?(1)

Quest.start("main_001")
Quest.complete("hunt_001")
Quest.active?("main_001")

Dialogue.start("Willkommen!")
Dialogue.choices("Ja", "Nein")
Dialogue.choice                 # Index der Auswahl (aus GameContext)

Map.id
Map.load(2)
Map.tint(0.2, 0.3, 0.4, 0.5)

Game.set_switch(7, true)
Game.variable(3)
```

## 4. Runtime-Anbindung

`runtime/src/bootstrap.cpp` ersetzt die bisherigen Hand-Bindings durch einen `RubyHost`
(`bind_ruby()`). Die Runtime konsumiert pro Frame:

| Anfrage | Aktion |
|---------|--------|
| `take_transfer` | Fade out → `load_map_into` bei Schwarz → Fade in |
| `take_map_load` | Transfer auf die Zielkarte |
| `take_scene_request` | Titel/Menü/Karte/SaveLoad/Kampf-Szene wechseln |
| `take_fade_request` | `ScreenFade` auslösen |
| `set_scene`/`update_map` | Live-Zeiger nach Kartenwechsel aktualisieren |

## 5. Stub-VM-Erweiterung

`eval_line` unterstützt jetzt auch die **Setter-Syntax** `Module.method = value`
(z. B. `Graphics.frame_rate = 30` → Host-Funktion `frame_rate=`), damit
Dokumentationsbeispiele auch headless laufen.

## 6. Tests

`tests/unit/ruby_host_test.cpp` (Test #17 `aether_ruby_host_test`):

- Audio: `bgm_play`/`bgm_stop` ändern echte Kanal-Zustände
- Wetter, Inventar (Id + Name), Gold, Quest, Schalter/Variablen
- Spieler: Position, Bewegung, `transfer` → `take_transfer`
- NPC: `find`/`set_position`/`say` (Dialog im GameContext)
- Enemy: `spawn`/`count`/`kill` in einer echten Scene
- Kamera: `move_to`/`look_at`/`zoom`
- Szenen/Karte/Fade: `goto`, `Map.load`, `Graphics.fade_out`, `Map.tint`, `Camera.shake`
- Stub-Setter `Graphics.frame_rate = 30`
- Input: `press?(:C)` nach `feed_key`

---

*Nächstes Modul: Dokumentations-Refresh*
