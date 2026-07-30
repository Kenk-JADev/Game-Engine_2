# Modul 11 – Editor

**Target:** `AetherEditor`  
**Status:** ImGui-GUI (mit GL) + Headless-CLI

## Bedienkonzept (verbindlich)

| Tab | Inhalt |
|-----|--------|
| **Projekt** | Neu / Öffnen / Speichern, Titel, Auflösung, Start |
| **Karte** | Palette, Objektliste, Eigenschaften, Auto-Kollision/Nav |
| **Datenbank** | Helden, Gegner, Items, Skills, System |
| **Events** | Befehlsliste (Nachricht, Schalter, Variable, Teleport, Skript, Warten) |
| **Skripte** | Ruby-Buffer, Speichern, Hot-Reload |
| **Testspiel** | Script-Boot + kurze Simulation + Nav-Smoke |
| **Export** | Paket mit Game-Binary |

**Nicht vorhanden (absichtlich):** Component-Liste, Add Component, Collider-/Rigidbody-Inspector.

## Start

```bash
# Mit Display + OpenGL-Dev-Paketen
AetherEditor --gui --project MyGame

# CI / ohne Display
AetherEditor --headless --new /tmp/G --testplay --max-frames 3
```

## Technik

- Dear ImGui + GLFW/OpenGL3-Backends wenn `AETHER_WITH_GLFW` + `AETHER_WITH_OPENGL`
- Sonst lauffähige Headless-Pipeline (NullWindow/NullRenderer)
