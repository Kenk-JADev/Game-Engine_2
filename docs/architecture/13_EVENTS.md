# Modul 13 – Eventsystem

**Namespace:** `aether::game`  
**Status:** Datenmodell + Interpreter implementiert

Visueller Event-Editor (ohne Programmierung) speichert Befehle als JSON.

## Befehlstypen

message · choice · set_switch · set_variable · conditional · transfer ·  
camera · weather · animation · quest · shop · battle · script · wait · comment

## Laufzeit

- `GameState` – Switches/Variables
- `EventInterpreter` – schrittweise Ausführung, Wait, Hooks für Transfer/Script
- `MapEvent` / `EventPage` – Trigger (action, touch, autorun, parallel)

---

*Editor-UI für Drag-Befehlslisten folgt.*
