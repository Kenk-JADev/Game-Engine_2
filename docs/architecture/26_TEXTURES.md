# Modul 26 – Textur-Pipeline (PNG/JPG → GPU)

**Namespace:** `aether::render` / `aether::res` / `aether::game`  \
**Status:** implementiert (stb_image → TextureData → GL-Textur → Shader-Sampler)

---

## 1. Ziel

Das Master-Prompt fordert **PNG/JPG** als unterstützte Assets. Bisher wurden
Texturen zwar geladen (stb_image → `TextureData`), aber **nie gerendert**:
Es gab keinen GPU-Upload, keine Sampler in den Shadern und keine Aufrufer von
`load_texture`. Dieses Modul schließt die komplette Kette:

```
PNG/JPG-Datei
   │  ResourceManager::load_texture (stb_image, Cache)
   ▼
TextureData (CPU, RGBA8)
   │  SceneObject.texture (attach_textures / Editor)
   ▼
Renderable.texture → DrawItem.texture
   │  Renderer::upload_texture (lazy, dedupliziert)
   ▼
GL-Textur (glTexImage2D + Mipmaps) → u_tex / u_has_tex
```

## 2. Änderungen

| Datei | Inhalt |
|-------|--------|
| `renderable.hpp` | `Renderable.texture` + `DrawItem.texture` (shared_ptr), `RenderStats.textures_uploaded` / `textured_draws` |
| `renderer.hpp/.cpp` | Virtuelles `upload_texture()`, Texture-Weitergabe in `build_draw_list` |
| `gl_renderer` | `upload_texture` (glGenTextures, Mipmaps, REPEAT), Binden pro Draw auf Textur-Einheit 0, Cache `gpu_textures_` |
| `null_renderer` | Zählt Uploads (dedupliziert) + texturierte Draws |
| `shader.cpp` + `engine/shaders/*` | Sampler `u_tex` + Flag `u_has_tex` in `lit_basic` und `unlit_color` |
| `scene.hpp/.cpp` | `SceneObject.texture` / `texture_path`, JSON-Roundtrip (`"texture": …`), Weitergabe in `collect_renderables` |
| `map_loader.hpp/.cpp` | `attach_textures()` lädt alle Objekt-Texturen über den ResourceManager; `load_map` nutzt den `resources`-Parameter |
| `runtime/bootstrap.cpp` | Runtime reicht ihren ResourceManager an `load_map` durch |
| `editor_app.cpp` | Eigenschaften-Panel: „Textur (log. Pfad)“-Feld mit Live-Load |

## 3. Karten-JSON

```json
{
  "name": "Boden",
  "type": 0,
  "mesh": "plane",
  "texture": "graphics/textures/checker.png"
}
```

Der Pfad ist ein **logischer Pfad** (VFS-Mount-Präfix, z. B. `graphics/…`),
wie er auch bei `ResourceManager::load_texture` verwendet wird.

## 4. GL-Details

- `GL_RGBA8` / `GL_RGB8` je nach Kanalzahl, `GL_UNPACK_ALIGNMENT=1`
- `glGenerateMipmap` + `GL_LINEAR_MIPMAP_LINEAR` (HD-4600-tauglich)
- `GL_REPEAT`-Wrapping (Boden/Flächen)
- Shader: `u_has_tex == 1` → `texture(u_tex, v_uv)` ersetzt die Albedo-Farbe
  (Alpha wird multiplikativ gemischt)

## 5. Tests

`tests/unit/texture_render_test.cpp` (Test #18 `aether_texture_test`):

- Eingebettetes 4×4-PNG → `load_texture` → korrekte Maße/Kanäle/Pixelwerte
- Scene-JSON-Roundtrip erhält `texture_path`
- `load_map` + `attach_textures` setzt `SceneObject.texture`
- NullRenderer: deduplizierter Upload + `stats.textured_draws` beim Zeichnen

---

*Nächstes Modul: Export-Tests & Release-Pipeline*
