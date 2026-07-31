# Modul 27 – Character-Sprites & Game Over

**Status:** implementiert

---

## 1. Ziel

- Charaktere sollen wie in klassischen RPG Makern als **Sprite-Billboards**
  erscheinen statt als farbige Würfel (`ActorData.character_graphic`-Gedanke).
- Verlorene Kämpfe enden in einem **Game-Over-Screen** (Neustart/Titel)
  statt im stillen „soft fail“ (HP=1).

## 2. Billboard-Sprites

### Mesh

`Mesh::create_quad(width, height)` erzeugt eine **senkrechte XY-Quad**
(Fußpunkt bei y=0, Fläche zeigt +Z) mit UVs passend zu PNG-Sprites
(UV (0,0) = oben-links).

### Kamera-Drehung

```cpp
Mat4 billboard_model(const Transform& t, const Vec3& camera_pos);
```

`renderer.cpp` verwendet diese Matrix in `build_draw_list`, wenn
`Renderable.billboard` gesetzt ist: **nur Yaw-Rotation** um die Hochachse –
Charaktere bleiben aufrecht (RPG-Stil), kein freies 360°-Kippen.

### Datenfluss

```
SceneObject.billboard + texture_path ("graphics/textures/hero.png")
   → JSON ("billboard": true, "texture": …)
   → assign_default_meshes: Character/Npc/Enemy = create_quad + billboard
   → attach_textures: TextureData laden
   → collect_renderables → Renderable.billboard/texture
   → build_draw_list: billboard_model + Textur
```

Standard-Meshes für Charaktere: Quad statt Würfel. Die bisherigen
Albedo-Farben bleiben als Tint/Fallback, falls keine Textur gesetzt ist.

### Demo-Assets

- `samples/demo_project/graphics/textures/hero.png` (32×48 Pixel-Art)
- `samples/demo_project/graphics/textures/elder.png` (32×48)
- `map001.json`: Player + Elder mit `texture` + `billboard`

## 3. Game Over

| Baustein | Inhalt |
|----------|--------|
| `GameOverScene` (scene_stack) | Auswahl „Neustart“ / „Titel“, Input up/down/confirm/cancel |
| Runtime-Kampfausgang | Niederlage (kein Flucht) → `scenes.replace(GameOverScene)` |
| Callback | `restart` → `start_new_game`; `title` → `TitleScene` |

Der bisherige „soft fail“ (HP=1 weiterlaufen) ist ersetzt; nur Flucht beendet
den Kampf ohne Game Over.

## 4. Tests

- `tests/unit/billboard_test.cpp` (Test #25): Quad-Mesh-Geometrie/UVs,
  `billboard_model`-Yaw (Kamera +Z / +X / 45°), Scene-JSON-Roundtrip,
  Render-Pfad mit Billboard-Objekt
- `tests/unit/gameplay_test.cpp`: GameOverScene-Callbacks (Enter → restart,
  Escape → title)

---

*Nächstes Modul: Animationen & Kamera-Events, Wetter-Partikel*
