# glTF, Script-IDE, Undo, Demo-Audio

## glTF/GLB

- Header-only **cgltf** unter `third_party/cgltf/`
- `aether::res::load_gltf_mesh` + `ResourceManager::load_mesh` für `.gltf`/`.glb`
- OBJ weiterhin eigener Loader; **FBX** seit Modul 24 über den eigenen FBX-Reader (ASCII + binär 7.x)

## Editor Undo

- Snapshot-JSON der Scene, max. 64 Schritte
- Menü: Rückgängig / Wiederholen
- Karte-Tab-Buttons

## Script-IDE

- Token-Highlight (Keywords, Strings, Comments, Numbers, Symbols)
- Autocomplete-Liste für Engine-API
- API-Dokumentationsspalte
- Zeilen-Debugger: Breakpoints, Debug Run, Step, Continue

## Demo

- WAV-Dateien unter `samples/demo_project/audio/`
- `map002.json` Waldrand + Portal zurück
- Portal im Dorf → Map 2
- Runtime registriert Audio-Clips automatisch

## Handbuch

`docs/user/HANDBUCH.md`
