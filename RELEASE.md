# AetherRPG Maker – Release & Erste Schritte

**Ziel:** Ein vollständiges eigenes 3D-RPG ohne Unity/Unreal/Godot erstellen.

## 1. Release-Build erstellen (einmalig)

```bash
cd /path/to/Game-Engine_2

# Empfohlen: Release-Build-Skript verwenden
chmod +x tools/build_release.sh
./tools/build_release.sh
```

Das Skript erzeugt:
```
dist/AetherRPG-Maker-Release/
├── bin/
│   ├── AetherEditor
│   └── Game
├── templates/
├── samples/
├── plugins/
├── assets/
└── README_PLAY.txt
```

**Alternative manuell (wenn du CMake hast):**

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release \
  -DAETHER_BUILD_EDITOR=ON -DAETHER_BUILD_RUNTIME=ON \
  -DAETHER_WITH_GLFW=ON -DAETHER_WITH_OPENGL=ON

cmake --build build-release -j$(nproc)
```

## 2. Dein erstes Spiel erstellen (5 Minuten)

### Schritt-für-Schritt

1. **Editor starten**
   ```bash
   cd dist/AetherRPG-Maker-Release/bin
   ./AetherEditor --gui
   ```

2. **Neues Projekt anlegen**
   - Im Tab **Projekt** → "Neues Projekt" eingeben (z.B. `/home/user/MyFirstRPG`)
   - Button **Anlegen**

3. **Karte bearbeiten (Drag & Drop)**
   - Wechsle zum Tab **Karte**
   - Links: Objekt-Palette (Prop, NPC, Gegner, Event, Boden)
   - Klicke auf ein Objekt → im Viewport klicken oder "Hier platzieren"
   - Mit Maus: Orbit (rechte Maustaste), Zoom (Mausrad), Auswählen (linke Maustaste)
   - Objekte verschieben im Platzier-Modus oder mit "Ziehen (LMB)"

4. **Event hinzufügen (ohne Programmierung)**
   - Tab **Events**
   - "Neues Event" anlegen
   - Befehle hinzufügen:
     - Nachricht
     - Teleport
     - Schalter / Variable
     - Skript (Ruby optional)
     - Kampf, Shop, Quest, Wetter

5. **Datenbank anpassen**
   - Tab **Datenbank**
   - Helden, Gegner, Items, Skills bearbeiten

6. **Ruby-Skripte (optional für Fortgeschrittene)**
   - Tab **Skripte**
   - `main.rb` bearbeiten (Hot-Reload möglich)

7. **Testen**
   - Tab **Testspiel** oder Menü → **Spiel → Testspiel**
   - Oder `F5`

8. **Exportieren (Game.exe-Paket)**
   - Tab **Export**
   - Zielordner angeben → **Exportieren**

9. **Fertig!**
   - Im Export-Ordner liegt `Game` (oder `Game.exe`)
   - Einfach ausführen → dein Spiel startet

## 3. Was der Spieler sieht (Runtime)

- Titelbildschirm
- Karte mit WASD + Pfeiltasten
- Interaktion mit `Z` / `Enter` / Linksklick
- Menü mit `Shift` / `V`
- Dialoge, Quests, Kampf, Shop, Wetter, Inventar

## 4. Unterstützte Formate

- Modelle: glTF, GLB, OBJ (FBX später)
- Texturen: PNG, JPG
- Audio: WAV, OGG, MP3
- Daten: JSON

## 5. Wichtige Tastenkürzel (Editor + Spiel)

| Aktion          | Editor          | Im Spiel          |
|-----------------|-----------------|-------------------|
| Bestätigen      | -               | Z / Enter / A     |
| Abbrechen       | -               | X / Esc / B       |
| Menü            | -               | Shift / V         |
| Bewegung        | Orbit + Zoom    | WASD + Pfeile     |
| Platzieren      | Linksklick      | -                 |

## 6. Projektstruktur (dein Spiel)

```
MyFirstRPG/
├── project.json
├── data/           # actors.json, items.json, ...
├── maps/           # map001.json ...
├── graphics/
├── audio/
├── scripts/main.rb
└── plugins/
```

## 7. Performance-Ziel

- 60 FPS auf älteren PCs (i3-4xxx + Intel HD 4600)
- Automatisches Frustum-Culling + LOD
- Einfache Shader

## 8. Nächste Schritte nach dem ersten Spiel

- Mehr Karten mit Teleport-Events
- Quests + Belohnungen
- Eigene 3D-Modelle importieren (glTF)
- Ruby für komplexe Logik
- Plugins schreiben (`plugin.json` + `main.rb`)

## 9. Troubleshooting

- **Kein Fenster?** → `--headless` nicht verwenden oder Display-Server starten
- **Keine Audio?** → miniaudio wird automatisch genutzt
- **Export ohne Game-Binary?** → zuerst den vollen Release bauen
- **Ruby nicht verfügbar?** → Stub-VM läuft trotzdem (API-Calls sind No-Op)

---

**Viel Spaß beim RPG-Machen!**

Die Engine ist bewusst **einfach** gehalten – genau wie klassische RPG Maker.

Bei Fragen: Siehe `docs/user/HANDBUCH.md` oder die Tabs im Editor.
