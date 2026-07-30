# Physik, Navigation & Szene

## Physik (`aether::phys`)

- AABB-Körper, Typen: Static / Dynamic / Kinematic / Trigger
- `make_body_from_mesh_bounds` – **automatisch** aus Mesh + Objektart
- `move_and_collide` mit Sub-Stepping (kein Tunneling)
- Overlap-Query für Event-Trigger

**Kein** manuelles Collider-Setup im Editor.

## Navigation (`aether::nav`)

- `bake_nav_grid` aus CollisionWorld
- A* (8-Nachbarn, Corner-Cut-Schutz)
- `NavAgent` folgt Waypoints

## Scene (`aether::scene`)

- `place(type, name, mesh, transform)` → Objekt + Auto-Kollision (+ Event-Shell)
- `collect_renderables` für den Renderer
- JSON-Serialisierung der Karte

## Editor-Anbindung

Karte-Tab: Palette (Prop/NPC/Gegner/Event/Boden) → Platzieren → Eigenschaften
ohne technische Collider-Felder. Buttons: „Navigation backen“, „Kollision neu“.
