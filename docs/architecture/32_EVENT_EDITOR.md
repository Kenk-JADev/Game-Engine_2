# Modul 32 – Event-Editor-Ausbau

**Status:** implementiert

---

## 1. Ziel

Der visuelle Event-Editor unterstützt jetzt den vollständigen
RPG-Maker-artigen Seiten-Workflow:

- **Mehrere Seiten** pro Event (letzte Seite mit erfüllten Bedingungen gewinnt)
- **Bedingungen-UI** pro Seite (Schalter / Variable mit Vergleichsoperator)
- **Befehls-Reihenfolge**: ▲ / ▼ verschieben, ⧉ duplizieren, löschen

## 2. Seiten

```
Seiten: [Seite 1] [Seite 2] [+ Seite] [Seite löschen]
```

- Jede Seite hat Name, Auslöser (Aktionstaste/Spieler-Touch/Event-Touch/
  Autorun/Parallel), Bedingungen und Befehle.
- Die Laufzeit (`MapEventRunner::select_page`) wählt die **letzte Seite mit
  erfüllten Bedingungen** – klassisches RPG-Maker-Verhalten.

## 3. Bedingungen (JSON in `page.conditions`)

| Feld | Bedeutung |
|------|-----------|
| `switch_id` / `switch_value` | Schalter muss gesetzt/nicht gesetzt sein |
| `variable_id` / `op` / `variable_value` | Variablen-Vergleich (`>=`, `==`, `>`, `<`, `<=`, `!=`) |

```json
{"switch_id": 5, "switch_value": true,
 "variable_id": 7, "op": ">=", "variable_value": 3}
```

## 4. Befehle

- Lesbare deutsche Labels („Nachricht: …“, „Teleport → Karte 1“, „Warten (30 Frames)“)
- Ausgewählten Befehl nach oben/unten verschieben, duplizieren, löschen

## 5. Tests

`tests/unit/event_runner_test.cpp`:

- Autorun-Seite mit **Schalter-Bedingung** feuert erst nach `set_switch(5, true)`
- **Variable-Bedingung** (`>=`) feuert erst ab Wert 3
- Ausgelöste Seite führt den Message-Befehl aus

---

*Nächstes Modul: Terrain-Editor (Höhen/Brush/Boden-Texturen)*
