# Modul 28 – Event-Animationen & Kamera-Steuerung

**Status:** implementiert

---

## 1. Ziel

Die Event-Befehle `PlayAnimation` und `ControlCamera` waren reine No-Ops
(„akzeptiert, aber nie ausgeführt“). Jetzt:

- **PlayAnimation** spielt eine Animation aus der Datenbank (`data/animations.json`)
  als Skalen-Puls auf dem Zielobjekt
- **ControlCamera** steuert Follow-Kamera-Offset, absolute Position/Blickziel
  und Kamera-Shake

## 2. PlayAnimation

| Baustein | Inhalt |
|----------|--------|
| `EventRequest::Kind::Animation` | neuer Request-Typ |
| Interpreter | `PlayAnimation` → `pending_ = {kind: Animation, params}` |
| Runtime | findet `AnimationData` per `animation_id`, Zielobjekt per Name (`target`-Parameter, Default: Player) |
| Effekt | `TransformTrack` mit Skalen-Puls (1 → 1.5 → 1), Dauer aus `frames/speed` |
| Anwendung | nur auf Nicht-Spieler-Objekte (Spieler-Transform wird vom Controller verwaltet) |

```json
{"type": "animation", "params": {"animation_id": 1, "target": "Elder"}}
```

## 3. ControlCamera

| Parameter | Wirkung |
|-----------|---------|
| `height` / `back` | Follow-Kamera-Offset (RPG-Stil) |
| `x` / `y` / `z` | absolute Kamera-Position |
| `tx` / `ty` / `tz` | Blickziel (Target) |
| `shake` / `duration` | Kamera-Shake (klingt linear ab) |

```json
{"type": "camera", "params": {"x": 0, "y": 20, "z": 25, "tx": 0, "ty": 0, "tz": 0,
                              "shake": 2, "duration": 0.5}}
```

Der Shake ist deterministisch (sin/cos über Frame-Zähler) und wird auch von
`Camera.shake(power, duration)` im RubyHost angestoßen (`take_shake` – einmalig).

## 4. Editor

- Event-Editor: neue Buttons **Animation** und **Kamera**
- Parameter-Bearbeitung: Animations-ID + Zielname; Kamera-Höhe/-Abstand,
  Shake-Test-Button

## 5. Runtime-CLI

Neu: `--log-level <level>` (trace|debug|info|warn|error) – Headless läuft sonst
auf „warn“ und verbirgt INFO-Logs (z. B. „PlayAnimation …“).

## 6. Tests

`tests/unit/event_runner_test.cpp`:

- `PlayAnimation` erzeugt `EventRequest::Kind::Animation` mit `animation_id`
- `ControlCamera` erzeugt `EventRequest::Kind::Camera`

End-to-End verifiziert: Autorun-Event spielt „Hit“ auf Elder (Runtime-Log).

---

*Nächstes Modul: Wetter-Partikel, echtes Testspiel, Editor-Ausbau*
