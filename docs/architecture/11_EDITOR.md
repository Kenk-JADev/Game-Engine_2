# Modul 11 – Editor (Grundgerüst)

**Target:** `AetherEditor`  
**Status:** CLI-Grundgerüst (GUI folgt)

---

## Bedienkonzept (verbindlich)

**Nur** diese Bereiche – **keine** Unity/Unreal-Component-UI:

| Bereich | Inhalt |
|---------|--------|
| Projekt | Neu / Öffnen / Speichern / Spieleinstellungen |
| Karte | 3D-Ansicht, Drag&Drop-Objekte, Events platzieren |
| Datenbank | Helden, Gegner, Items, Skills, … |
| Events | Visueller Event-Editor |
| Skripte | Ruby-IDE (Highlight, Complete, Debugger) |
| Testspiel | Runtime im Debug-Modus |
| Export | Game-Paket erzeugen |

Objekte per Drag&Drop – Kollision/Navigation **automatisch**.

---

## Phase-1 CLI

```bash
AetherEditor --new /path/MyGame
AetherEditor --project /path/MyGame --testplay
AetherEditor --help
```

---

*GUI-Widgets und Map-Editor folgen in späteren Iterationen.*
