# Modul 24 – FBX-Import (eigener Reader)

**Namespace:** `aether::res`  \
**Dateien:** `engine/include/aether/res/fbx_loader.hpp`, `engine/src/res/fbx_loader.cpp`  \
**Status:** implementiert (ASCII + binär 7.x)

---

## 1. Ziel

Das Master-Prompt fordert als Asset-Formate: **glTF, GLB, FBX, OBJ, PNG, JPG, WAV, OGG, MP3**.
glTF/GLB liefen bereits über cgltf, OBJ über einen eigenen Loader – **FBX fehlte**.
Statt einer externen SDK (assimp o. Ä.) ist der Import vollständig eigenständig implementiert:

- **ASCII-FBX** (7.x-Schreibweise, wie Blender/FBX-SDK sie exportieren)
- **Binäres FBX 7.x** (Versionen 7000–7400)
- Kein proprietärer Code, keine externen Abhängigkeiten

## 2. Architektur

```
.fbx-Datei
   │
   ├─ "Kaydara FBX Binary  " → BinaryReader (Node-Records)
   │      EndOffset (relativ!), NumProperties, PropertyListLen,
   │      NameLen/Name, typisierte Properties, Kinder
   │
   └─ sonst → ASCII-Parser (rekursiv, zeilenbasiert, Node-Stack)
         "Name: *N { a: … }" → ein Array-Property (wie im Binärformat)

        beides → gemeinsamer FbxNode-Baum
                 │
                 └─ build_mesh():
                      Geometry (Vertices, PolygonVertexIndex)
                    + Model (Properties70: Lcl Translation/Rotation/Scaling)
                    + Connections (Model → Geometry)
                    + LayerElementNormal / LayerElementUV
                    → render::Mesh (Fan-Triangulation, flache Normalen-Fallback)
```

### Binär-Format-Details

- Header: `Kaydara FBX Binary  ` + `\0\x1A\0` + Version (u32 LE)
- **`EndOffset` ist relativ zum Record-Anfang** (häufige Fehlerquelle!)
- Property-Typen: `Y C I F D L S R`, Arrays `f d i l b`
- Komprimierte Arrays (`encoding != 0`, zlib) werden mit klarer Warnung abgelehnt –
  Export ohne Kompression verwenden (Blender schreibt standardmäßig unkomprimiert).

### ASCII-Format-Details

- Rekursiver Parser; `}` schließt den aktuellen Knoten
- Array-Blöcke `Name: *N { a: … }` werden zu **einem** Array-Property zusammengefasst
  (Wertezeilen dürfen mit Komma über mehrere Zeilen laufen)

## 3. Geometrie-Extraktion

| Element | Quelle |
|---------|--------|
| Positionen | `Geometry::Vertices` (f64/f32-Array) |
| Polygon-Indizes | `Geometry::PolygonVertexIndex` (negativer letzter Wert = Polygende) |
| Normalen | `LayerElementNormal` – `Direct`/`IndexToDirect`, Mapping `ByPolygonVertex`/`ByVertice`/`AllSame` |
| UVs | `LayerElementUV` + `UVIndex` |
| Transform | `Model::Properties70` – `P: "Lcl Translation/Rotation/Scaling", …` |
| Zuordnung | `Connections::C: "OO", ModelId, GeometryId` |

- **Fan-Triangulation**: Polygone mit n Ecken → n−2 Dreiecke (3 eigene Vertices pro Dreieck)
- **Flache Normalen** werden automatisch berechnet, wenn das FBX keine enthält

## 4. Integration

`ResourceManager::read_mesh_stub` dispatcht jetzt nach Endung:

| Endung | Loader |
|--------|--------|
| `.gltf` / `.glb` | cgltf |
| `.fbx` | **eigener FBX-Reader** |
| `.obj` | eigener OBJ-Loader |

## 5. Tests

`tests/unit/fbx_loader_test.cpp` (Test #16 `aether_fbx_test`):

- ASCII-Dreieck mit Normalen/UVs + Model-Translation → 3 Vertices, 1 Dreieck
- Binär-Quad (FBX 7400, mit testseitigem Mini-Encoder) → 6 Vertices, 2 Dreiecke
- ResourceManager-Integration über `load_mesh("m/tri.fbx")`
- Fehlerpfade (fehlende Datei, Müll-Input)

## 6. Bekannte Grenzen (dokumentiert)

- Kein Skinning, keine FBX-Animationen, Kameras, Lichter
- Keine zlib-komprimierten Array-Properties
- Ein Mesh je Aufruf (alle Model-Geometrien werden zusammengeführt)

---

*Nächstes Modul: Ruby-Host-API*
