# Modul 5 – Fenster (Window)

**Namespace:** `aether::window`  
**Status:** implementiert

---

## 1. Architektur

Das Window-Modul kapselt OS-Fenster und den OpenGL-Kontext hinter einer stabilen API.

```
Window (abstrakt)
├── GlfwWindow   # Desktop (AETHER_WITH_GLFW)
└── NullWindow   # Headless / CI / fehlende GUI-Deps
```

| Komponente | Aufgabe |
|------------|---------|
| `Window` | Öffentliche API: Größe, Titel, VSync, Fullscreen, Poll, Swap |
| `WindowSystem` | Einmal-Init/Terminate des Backends (GLFW) |
| `WindowDesc` | Erstellungsparameter (aus `GraphicsConfig` ableitbar) |
| Events | Resize, FramebufferResize, Close, Focus, ContentScale, Drop |

**Designprinzip:** Der Spiele-Autor konfiguriert nur Titel/Auflösung/Vollbild im Projekt – keine „Collider“- oder Component-UI. Technische Fensterdetails bleiben Engine-intern.

---

## 2. Dateien

```
engine/include/aether/window/
  window.hpp
  window_events.hpp
  window_module.hpp

engine/src/window/
  window.cpp
  null_window.hpp / .cpp
  glfw_window.hpp / .cpp   # nur mit GLFW
```

---

## 3. Backends

### GLFW (`AETHER_WITH_GLFW`)

- OpenGL **3.3 Core Profile** (Intel HD 4600-kompatibel)
- Double-Buffer, optional Debug-Context
- Fullscreen über Primary Monitor + VideoMode
- Callbacks → `EventBus`

### NullWindow

- Kein OS-Fenster, kein GL-Kontext
- Ideal für Unit-Tests, Tools, Headless-CI
- API-Verhalten analog (Resize/Close-Events werden synthetisch ausgelöst)

**Auto-Fallback:** Schlägt `glfwInit` / `glfwCreateWindow` fehl, wechselt die Factory auf `NullWindow`.

**Build ohne X11-Dev-Pakete:** CMake deaktiviert GLFW automatisch und baut nur NullWindow.

---

## 4. Nutzung

```cpp
#include <aether/window/window.hpp>

using namespace aether::window;

WindowDesc desc;
desc.title = "My Game";
desc.width = 1280;
desc.height = 720;
desc.vsync = true;

auto window = Window::create(desc, &ctx->events());
// oder erzwungen headless:
// auto window = Window::create(desc, &ctx->events(), WindowBackend::Null);

while (!window->should_close()) {
    window->poll_events();
    // update + render …
    window->swap_buffers();
}
window.reset();
WindowSystem::terminate();
```

### Events

```cpp
ctx->events().subscribe<WindowResizeEvent>([](const WindowResizeEvent& e) {
    // Viewport anpassen (Renderer-Modul)
});
ctx->events().subscribe<WindowCloseEvent>([](const WindowCloseEvent&) {
    // Spiel beenden
});
```

---

## 5. CMake

```bash
# Standard: GLFW wenn X11+GL Dev vorhanden
cmake -S . -B build

# Explizit ohne GLFW
cmake -S . -B build -DAETHER_WITH_GLFW=OFF
```

Benötigte Systempakete (Linux) für GLFW:

```
libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev
```

---

## 6. Tests

`tests/unit/window_test.cpp` – NullWindow-API, Events, Lebenszyklus.

---

## 7. Nächstes Modul

**Renderer** – OpenGL 3.3 Forward-Renderer, Shader, Mesh, Camera, Frustum-Culling, LOD, Clear/Swap-Anbindung an Window.

---

*Modul 5 abgeschlossen.*
