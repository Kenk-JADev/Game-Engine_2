# Gameplay & Runtime-Loop

## Runtime-Ablauf (`Game`)

1. `project.json` laden  
2. `EngineContext` + Window/Renderer/Input/Audio  
3. Datenbank aus `data/`  
4. Karte laden (`maps/mapXXX.json`) oder Default-Karte  
5. Player binden (Auto-Kollision)  
6. Ruby-API + Plugins  
7. Main-Loop:
   - `poll_events` + Input (GLFW-Callbacks wenn verfügbar)
   - Fixed-Update: Bewegung, Interaktion (Confirm → Event)
   - EventInterpreter (Message/Switch/Transfer/Script/…)
   - Follow-Kamera, Wetter-Tint
   - Render (Frustum-Culling + LOD)
   - Swap

## Steuerung

| Action | Tasten | Wirkung |
|--------|--------|---------|
| up/down/left/right | WASD / Pfeile | Laufen |
| confirm | Z / Space / Enter / C | Event starten |
| cancel | X / Escape | Beenden (Runtime) |

## Default-Karte

- Boden (Plane)
- Player (blau)
- NPC „Elder“ mit Dialog-Event
- Felsen (Hindernis)
- Event-Schild

## Module

| Modul | Namespace |
|-------|-----------|
| PlayerController / FollowCamera | `aether::game` |
| WeatherSystem | `aether::game` |
| MapLoader | `aether::game` |
| Animator / Tween | `aether::anim` |
| Collision / Nav | `aether::phys` / `aether::nav` |

## Tests

`aether_gameplay_test` – Tween, Wetter, Map, Player-Bewegung, Interaktion, Kamera.
