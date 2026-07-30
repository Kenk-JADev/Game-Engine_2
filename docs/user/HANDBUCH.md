# AetherRPG Maker – Kurzanleitung

## 1. Installation / Build

```bash
# Abhängigkeiten (Linux, für Fenster + OpenGL)
sudo apt install build-essential cmake ninja-build git \
  libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
  libgl1-mesa-dev libasound2-dev

# Optional mruby
sudo apt install ruby ruby-dev bison

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DAETHER_WITH_GLFW=ON -DAETHER_WITH_OPENGL=ON \
  -DAETHER_WITH_MINIAUDIO=ON -DAETHER_WITH_MRUBY=ON
cmake --build build -j
```

Binaries: `build/bin/AetherEditor`, `build/bin/Game`

## 2. Editor – die sieben Bereiche

| Tab | Was Sie tun |
|-----|-------------|
| **Projekt** | Neu anlegen, öffnen, Titel/Auflösung |
| **Karte** | Objekte platzieren, verschieben, Navigation backen |
| **Datenbank** | Helden, Gegner, Items, Skills |
| **Events** | Dialoge, Schalter, Kampf, Shop, Quest … |
| **Skripte** | Ruby-Logik, Highlight, API-Hilfe |
| **Testspiel** | Kurzer Smoke-Lauf |
| **Export** | Spielpaket erzeugen |

**Wichtig:** Es gibt keine Component-/Collider-Liste. Kollision und Laufen werden automatisch eingerichtet.

### Karte bedienen

1. Objekt in der **Palette** wählen → **Platzieren**
2. In der Liste auswählen → Position/Größe im Inspector
3. **Navigation backen** für NPC-Pfade
4. **Rückgängig / Wiederholen** über Menü oder Buttons

### Events (ohne Code)

Am Objekt (z. B. NPC) im Tab Events:

- Nachricht, Schalter, Variable, Teleport  
- Auswahl, Kampf, Shop, Quest, Wetter, Skript  

### Skripte (Ruby)

Nur Spiellogik. Beispiele:

```ruby
Audio.bgm_play("Theme1", 80, 100)
Player.transfer(1, 5.0, 0.0, 5.0, 2)
Inventory.gain(1, 3)
Quest.start("main_001")
Weather.set("rain", 5)
```

Im Editor: Highlight-Vorschau, Autocomplete-Liste, API-Doku-Spalte, Hot-Reload.

## 3. Spiel steuern (Game)

| Taste | Aktion |
|-------|--------|
| WASD / Pfeile | Laufen / Menüwahl |
| Z / Leertaste / Enter | Bestätigen, sprechen |
| Shift | Menü (Items, Speichern, Laden) |
| Bild-Ab | Demo-Kampf |
| Esc / X | Zurück |

Ablauf: **Titel → Neues Spiel → Karte → NPCs → Kampf/Shop/Quest → Speichern**

```bash
./build/bin/Game --project samples/demo_project
./build/bin/Game --project /pfad/MeinSpiel
```

## 4. Projektordner

```
MeinSpiel/
  project.json
  data/          # actors, enemies, items, skills, system
  maps/          # map001.json …
  graphics/
  audio/bgm|bgs|me|se
  scripts/main.rb
  plugins/
  saves/         # Spielstände (zur Laufzeit)
```

## 5. Assets

| Format | Unterstützung |
|--------|----------------|
| PNG, JPG | ja (stb_image) |
| OBJ | ja |
| glTF, GLB | ja (cgltf) |
| FBX | nein (Cube-Fallback) |
| WAV, OGG, MP3 | ja (miniaudio); Demo enthält WAV-Beispiele |

### Script-Debugger

Im Tab **Skripte**: Breakpoint-Zeile setzen → **BP+** → **Debug Run** → bei Stop **Step** / **Continue**.

## 6. Plugins

```
plugins/MeinPlugin/
  plugin.json
  main.rb
  assets/
```

## 7. Export

Im Editor-Tab **Export** Zielordner wählen → enthält `Game`, `project.json`, Daten und Assets.

## 8. Tipps

- Anfänger: nur Karte + Datenbank + Events  
- Fortgeschritten: Ruby + Plugins  
- CI/Headless: `Game --headless --max-frames 30`  
- Kein Unity-Workflow – denken Sie in **Karten, Events, Datenbank**
