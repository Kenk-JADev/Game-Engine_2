# AetherRPG Maker

**Die eigene 3D-RPG-Maker Engine** – komplett ohne Unity, Unreal, Godot oder RPG Maker XP.

Erstelle vollständige 3D-Rollenspiele mit Drag & Drop, visuellem Event-Editor und **ohne Programmierung**.

---

## ✅ **So startest du OHNE Terminal / CMD** (einfachster Weg)

### 1. Klicke hier: [Releases](https://github.com/Kenk-JADev/Game-Engine_2/releases/latest)

### 2. Lade die Datei für dein System herunter

| Windows                          | Linux                              |
|----------------------------------|------------------------------------|
| `AetherRPG-Maker-Windows-x64.zip` | `AetherRPG-Maker-Linux-x64.tar.gz` |

### 3. Entpacke die Datei

### 4. Doppelklicke auf:

- **Windows**: `start_editor.bat`
- **Linux**: `start_editor.sh`

**Fertig!** Der Editor startet sofort.

---

## 🚀 **Einfach herunterladen & sofort loslegen** (kein Terminal nötig!)

### Für Windows-Nutzer (empfohlen):

1. Gehe zu **[Releases](https://github.com/Kenk-JADev/Game-Engine_2/releases)**
2. Lade die neueste `AetherRPG-Maker-Windows-x64.zip` herunter
3. Entpacke die Zip-Datei
4. **Doppelklick** auf `start_editor.bat`

Fertig! Der Editor startet direkt.

### Für Linux-Nutzer:

1. Gehe zu **[Releases](https://github.com/Kenk-JADev/Game-Engine_2/releases)**
2. Lade `AetherRPG-Maker-Linux-x64.tar.gz` herunter
3. Entpacke das Archiv
4. Starte mit Doppelklick oder Terminal: `./start_editor.sh`

---

## Was du ohne Programmierung machen kannst

- 3D-Karten per Drag & Drop erstellen (Objekte einfach anklicken + platzieren)
- Visueller Event-Editor (Dialoge, Teleporter, Kämpfe, Shops, Quests, Wetter...)
- Datenbank für Helden, Gegner, Items
- Automatische Kollision & Navigation
- Export als eigenständiges Spiel (`Game.exe` / `Game`)

Fortgeschrittene können optional **Ruby** für eigene Logik nutzen.

---

## 📦 Releases (einfachste Methode)

Gehe immer zu den **[Releases](https://github.com/Kenk-JADev/Game-Engine_2/releases)**.

Jedes Release enthält fertige, portable Pakete:
- Windows: `AetherRPG-Maker-Windows-x64.zip` → `start_editor.bat`
- Linux: `AetherRPG-Maker-Linux-x64.tar.gz` → `start_editor.sh`

**Kein Kompilieren, kein CMake, kein `make` nötig** für normale Benutzer.

---

## Für Entwickler / Build from Source

Nur wenn du selbst etwas ändern willst:

```bash
git clone https://github.com/Kenk-JADev/Game-Engine_2.git
cd Game-Engine_2
make package          # oder ./tools/build_release.sh
```

---

## Editor Tabs (genau wie klassische RPG Maker)

| Tab        | Was du hier machst                          |
|------------|---------------------------------------------|
| **Projekt**   | Neues Spiel anlegen / öffnen                |
| **Karte**     | 3D-Welt per Drag & Drop gestalten           |
| **Datenbank** | Helden, Gegner, Items, Skills               |
| **Events**    | Visuelle Events (kein Code nötig)           |
| **Skripte**   | Optional: Ruby für Fortgeschrittene         |
| **Testspiel** | Schnell testen                              |
| **Export**    | Fertiges Spiel als `Game.exe` exportieren   |

---

## Schnellstart: Dein erstes RPG in 10 Minuten

1. Editor per `start_editor.bat` / `.sh` starten
2. **Projekt** → Neues Projekt anlegen
3. **Karte** → Objekte aus der Palette anklicken und in die Welt klicken
4. **Events** → "Neues Event" → Nachrichten, Teleport, Kampf etc. hinzufügen
5. **Testspiel** oder F5 drücken
6. **Export** → Dein Spiel ist fertig!

---

## Systemanforderungen (für Spieler)

- Windows 10/11 oder Linux
- Beliebige Grafikkarte (auch alte Intel HD Graphics)
- 4 GB RAM reichen völlig aus

---

## Lizenz

MIT License

---

**Viel Spaß beim Erstellen deines eigenen 3D-RPGs!**

Alles, was du brauchst, ist ein Download von den Releases und ein Doppelklick.
