# Modul 6 – Renderer

**Namespace:** `aether::render`  
**Status:** implementiert (Null + OpenGL 3.3/GLAD optional; Culling, LOD, GLSL 330)

---

## 1. Architektur

```
Renderer
├── build_draw_list()     # Frustum-Culling + LOD + Sortierung
├── NullRenderer          # Headless / CI / Stats
└── GlRenderer (später)   # OpenGL 3.3 Forward
```

| Komponente | Aufgabe |
|------------|---------|
| `math.hpp` | Vec/Mat/Quat, AABB, Transform (glm) |
| `Camera` | Perspektiv/Ortho, View/Proj, Frustum |
| `Mesh` / `MeshLod` | CPU-Geometrie, LOD-Stufen, Bounds |
| `Material` | Einfaches Albedo/Alpha (kein PBR-Zwang) |
| `ShaderProgram` | GLSL 330 Quellen + GPU-Handle |
| `Renderable` | Einreichobjekt (Mesh + Material + Transform) |
| `Renderer` | Frame, Culling, LOD, Draw, Stats |

**Performance-Ziele (Master-Prompt):**

- Automatisches **Frustum Culling**
- Automatisches **LOD**
- Einfache, effiziente Shader (GLSL 330)
- Forward Rendering, **kein** Raytracing
- Ziel: 60 FPS auf HD 4600-Klasse

---

## 2. Dateien

```
engine/include/aether/render/
  math.hpp, color.hpp, camera.hpp, mesh.hpp,
  material.hpp, shader.hpp, renderable.hpp,
  renderer.hpp, render_module.hpp

engine/src/render/
  camera.cpp, mesh.cpp, shader.cpp,
  renderer.cpp, null_renderer.cpp

engine/shaders/
  unlit_color.vert/.frag
  lit_basic.vert/.frag
```

---

## 3. Pipeline (pro Frame)

1. `begin_frame()` – Stats reset, Clear (GL)
2. `draw(camera, renderables)`
   - World-AABB aus lokalem Bounds × Model-Matrix
   - Kugeltest gegen Frustum → cull
   - LOD über Kameradistanz (`MeshLod::max_distance`)
   - Sort: Opaque front-to-back, Transparent back-to-front
   - Backend zeichnet DrawItems
3. `end_frame()` – Flush  
4. `Window::swap_buffers()`

---

## 4. Nutzung

```cpp
#include <aether/render/render_module.hpp>

using namespace aether::render;

RendererDesc rd;
rd.backend = RendererBackend::Null; // oder OpenGL wenn verfügbar
auto renderer = Renderer::create(rd, window.get());

auto cube = Mesh::create_cube(1.0f);
// LOD0 bis 20m, LOD1 danach
MeshLod hi = cube->lod(0);
hi.max_distance = 20.f;
cube->set_lod(0, std::move(hi));
auto tmp = Mesh::create_cube(1.0f);
MeshLod lo = tmp->lod(0);
lo.max_distance = 1e9f;
cube->set_lod(1, std::move(lo));
renderer->upload_mesh(*cube);

Camera cam;
cam.set_perspective(45.f, 16.f/9.f, 0.1f, 500.f);
cam.look_at({0,5,12}, {0,0,0}, {0,1,0});

std::vector<Renderable> items;
Renderable r;
r.mesh = cube;
r.transform.position = {0,0.5f,0};
r.material.albedo = Color::white();
items.push_back(r);

renderer->begin_frame();
renderer->draw(cam, items);
renderer->end_frame();
// window->swap_buffers();
```

---

## 5. Shader (Built-in)

| Name | Beschreibung |
|------|--------------|
| `unlit_color` | Vertex-Color × Albedo |
| `lit_basic` | 1x Directional Light + Ambient |

Beide: **GLSL 330 core**, wenige Uniforms, HD-4600-tauglich.

---

## 6. CMake / Deps

- **glm** (FetchContent, Pflicht)
- OpenGL-Backend optional (`AETHER_WITH_OPENGL`, benötigt GLFW)

---

## 7. Tests

`tests/unit/render_test.cpp`

- AABB-Transform
- LOD-Auswahl
- Frustum-Culling + Stats
- Built-in-Shader-Registrierung

---

## 8. Nächstes Modul

**Input** – Tastatur/Maus/Gamepad, Action-Mapping, Anbindung an Window-Backend.

---

*Modul 6 abgeschlossen.*
