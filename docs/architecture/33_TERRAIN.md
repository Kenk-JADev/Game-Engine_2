# Modul 33 – Terrain-Editor (Höhenfeld + Boden-Textur)

**Status:** implementiert

---

## 1. Ziel

Der Karten-Editor konnte nur Objekte platzieren. Jetzt gibt es ein
**Höhenfeld-Terrain** mit Pinsel-Werkzeugen – im RPG-Maker-Geist einfach:
„Heben / Senken / Glätten“ per Klick-Ziehen im Viewport.

## 2. Datenmodell

```json
"terrain": {
  "width": 8, "depth": 8, "cell": 4.0,
  "texture": "graphics/textures/checker.png",
  "heights": [0, 0.12, …]   // (width+1)×(depth+1) Eck-Höhen, row-major
}
```

- Gespeichert in der Karten-JSON (Scene::to_json/from_json)
- `TerrainData::height_at(x, z)` – bilineare Interpolation (außerhalb → 0)
- Grid liegt zentriert um (0,0); Zellgröße `cell`

## 3. Rendering

`Mesh::create_terrain(width, depth, cell, heights)`:

- Grid-Mesh aus Quads (4 Vertices/Zelle), Index-Dreiecke wie `create_plane`
- **Flache Normalen** aus der Geometrie (invertiert zur Windungsrichtung,
  zeigt nach oben → korrektes Licht)
- `collect_renderables` hängt das Terrain als **letztes** Renderable an
  (Boden unter den Objekten), Textur über `terrain_texture`

## 4. Spieler

`PlayerController::update_movement` setzt die Y-Position automatisch auf
`terrain.height_at(x, z) + cfg.height` – Charaktere laufen über Hügel, ohne
dass der Ersteller Physik konfigurieren muss (Master-Prompt: Kollision &
Standardverhalten automatisch).

## 5. Editor (Karten-Tab → Eigenschaften → Terrain)

| Steuerung | Aktion |
|-----------|--------|
| „Terrain anlegen“ | Grid (Breite/Tiefe) initialisieren |
| „Pinsel aktiv“ | LMB im Viewport malt (Ray auf Bodenebene → Zelle) |
| Pinsel-Modus | Heben / Senken / Glätten (Radius + Stärke, Falloff) |
| „Textur übernehmen“ | Boden-Textur (log. Pfad) laden |
| „Höhen zurücksetzen“ | alle Höhen = 0 |
| „Terrain entfernen“ | Terrain aus der Karte löschen |

Jede Pinsel-Drag-Session erzeugt einen Undo-Schritt (Snapshot-Undo).

## 6. Demo

`map001.json` hat jetzt einen sanften Hügel in der Kartenmitte
(8×8 Zellen à 4 m, max. ~1 m) mit Checker-Textur.

## 7. Tests

`tests/unit/terrain_test.cpp` (Test #28 `aether_terrain_test`):

- `TerrainData`: reset/valid/height_at (bilinear, außerhalb = 0)
- `create_terrain`: Vertex/Index-Zahl, Bounds, flache Normalen nach oben
- Scene-JSON-Roundtrip + `load_map` baut `terrain_mesh`
- `collect_renderables` enthält das Terrain
- `PlayerController`: Y folgt dem Terrain bei Bewegung

---

*Alle identifizierten Lücken sind geschlossen.*
