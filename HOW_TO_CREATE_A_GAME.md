# SO ERSTELLST DU DEIN ERSTES RPG MIT AetherRPG Maker

## Voraussetzungen
- 64-Bit Linux oder Windows
- Für vollen Editor: OpenGL-fähige Grafik + X11/Wayland (oder Windows)
- Mindestens 4 GB RAM (8 GB empfohlen)

## Schnellstart (empfohlen)

### 1. Release bauen

```bash
cd /path/to/Game-Engine_2

# Einmalig:
make package
# oder
./tools/build_release.sh
```

Das erzeugt `dist/AetherRPG-Maker-Release/`

### 2. Editor starten

```bash
cd dist/AetherRPG-Maker-Release/bin
./AetherEditor --gui
```

### 3. Neues Spiel anlegen

1. Tab **Projekt**
2. Bei "Neues Projekt" einen Pfad eingeben, z.B.:
   - `/home/deinname/MeinErstesRPG`
3. Button **Anlegen**

### 4. Karte gestalten (Drag & Drop – keine Programmierung nötig)

- Wechsle zu Tab **Karte**
- Links siehst du die **Objekt-Palette**:
  - Prop (Würfel)
  - NPC
  - Gegner
  - Event
  - Boden
- Klicke ein Objekt an
- Im großen 3D-Viewport klicken → Objekt wird platziert
- Mit rechter Maustaste drehen, Mausrad zoomen
- Objekte auswählen und im Inspector unten verschieben/skaliert

**Automatisch:**
- Kollision wird gesetzt
- Navigation wird vorbereitet (Button "Navigation backen")

### 5. Events ohne Code erstellen

Tab **Events** → "Neues Event"

Mögliche Befehle (per Buttons):
- Nachricht (Dialog)
- Auswahl (Ja/Nein)
- Teleport
- Schalter / Variable
- Kampf starten
- Shop
- Quest starten
- Wetter ändern
- Ruby-Skript (optional)

### 6. Testen

- Menü **Spiel → Testspiel** oder Tab **Testspiel**
- Oder einfach `F5`

Steuerung im Testspiel:
- WASD / Pfeiltasten = bewegen
- Z / Enter / Linksklick = interagieren
- Shift = Menü

### 7. Exportieren (fertiges Spiel)

1. Tab **Export**
2. Zielordner angeben (z.B. `~/MeinRPG_Export`)
3. **Exportieren**

Danach enthält der Ordner:
- `Game` (oder `Game.exe`)
- Alle deine Daten, Karten, Scripts
- Einfach `Game` starten → dein Spiel läuft

## Was du alles ohne Programmierung machen kannst

- Komplette 3D-Karten mit Objekten
- Dialoge + Auswahlen
- Quests
- Händler-Shops
- Zufallskämpfe / Trigger-Kämpfe
- Teleporter
- Wetter (Regen, Schnee…)
- Inventar-System
- Level / EXP / Gold
- Mehrere Karten

## Fortgeschritten: Ruby

Nur wenn du willst:
- Tab **Skripte**
- `main.rb` bearbeiten
- Hot-Reload mit "Ausführen / Hot-Reload"

Beispiel:
```ruby
Audio.bgm_play("Theme1", 80, 100)
Weather.set("rain", 6)
```

## Wichtige Tipps

- Immer zuerst ein **Projekt** anlegen
- Speichere regelmäßig (Ctrl+S oder Button)
- Im Karteneditor: "Undo" und "Redo" sind verfügbar
- Export kopiert automatisch alles Nötige

## Wenn etwas nicht funktioniert

- Kein Fenster? → Stelle sicher, dass du auf einem System mit Display bist
- Keine Audio? → Normal in headless, im vollen Build sollte es funktionieren
- Export hat kein Game? → `make package` oder `make release` zuerst ausführen

---

**Du bist jetzt bereit, dein eigenes 3D-RPG zu machen!**

Viel Erfolg!
