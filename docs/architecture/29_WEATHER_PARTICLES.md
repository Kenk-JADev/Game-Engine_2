# Modul 29 – Wetter-Partikel (sichtbarer Niederschlag)

**Status:** implementiert

---

## 1. Ziel

Regen/Sturm/Schnee/Nebel waren bisher nur ein Farb-Tint. Jetzt erzeugt das
Wettersystem **sichtbare Partikel** um den Spieler – gerendert als kleine
Billboard-Quads über die vorhandene Render-Pipeline (kein Partikel-System
von Drittanbietern).

## 2. Architektur

```
WeatherSystem
├── Tint (bisher, bleibt)
└── Partikel (neu)
    ├── sync_particle_count()  – Anzahl = power × 25 (0–250)
    ├── respawn_particle()     – Spawn in 22 m Radius, oben (y+12..18)
    ├── update(dt, center)     – Bewegung + Recycling unter y<0
    └── particles()            – Positionen für den Renderer
```

| Wetter | Verhalten |
|--------|-----------|
| Rain  | schnell nach unten (−14…−18 m/s), leichter Drift |
| Storm | schneller + Seitenwind (−20…−25 m/s) |
| Snow  | langsam (−1,2…−2,2 m/s), Sinus-Schwanken |
| Fog   | langsame Schwaden um den Spieler (26 m Radius), transparent |

Partikel-Bewegung ist **deterministisch** (Hash-Funktion auf Partikel-Index) –
kein Zufallsgenerator → reproduzierbare Tests.

## 3. Rendering (Runtime)

- Ein geteiltes `Mesh::create_quad(1,1)` (`RuntimeState::particle_mesh`)
- Pro Partikel ein Renderable mit Skalierung:
  - Regen: dünner Tropfen (0,06 × 0,35)
  - Schnee: Quadrat (0,12)
  - Nebel: großer transparenter Schwaden (2,5 × 1,2), `transparent`
- `billboard = true` → Partikel drehen sich zur Kamera

## 4. Tests

`tests/unit/gameplay_test.cpp`:

- Regen erzeugt Partikel; Partikel **fallen** (y sinkt); Spawn nahe am Zentrum
- `clear()` → 0 Partikel
- Schnee deutlich langsamer als Regen
- Nebel erzeugt Partikel

---

*Nächstes Modul: echtes Testspiel, Script-IDE, Event-Editor, Terrain*
