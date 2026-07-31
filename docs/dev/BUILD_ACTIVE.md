# Build Active – Alles aktiv (Engine + Editor + Runtime + Workflow + Docs)

**Status:** Milestone 01 vollständig integriert. Alle Komponenten aktiv.

## Aktivierte Komponenten

| Komponente | Status | Datei / Beweis |
|------------|--------|----------------|
| **Engine Core** | ✅ Aktiv | `engine/src/game/event_bridge.cpp` (CMakeLists.txt) |
| **Event Interpreter** | ✅ Aktiv | `engine/src/game/event_system.cpp` + `bootstrap.cpp` Link |
| **Editor (UI)** | ✅ Aktiv wenn mit GLFW/ImGui gebaut | `editor/src/ui/editor_app.cpp` |
| **Runtime (Game.exe)** | ✅ Aktiv | `runtime/src/bootstrap.cpp` + `EventBridge`-Integration |
| **Plugin-System** | ✅ Aktiv | `plugin_loader.cpp` + `ruby_module` in Bootstrap |
| **Ruby / mruby** | ✅ Aktiv wenn `-DAETHER_WITH_MRUBY=ON` | `ruby/ruby_vm.hpp` + `stub` oder echt |
| **Workflow / CI** | ✅ Aktiv | `.github/workflows/ci-build.yml` + `docs/dev/github-workflows/ci-build-active.yml` |
| **Smoke-Test** | ✅ Aktiv | `tests/integration/test_event_bridge.cpp` + `compile_smoke.sh` |
| **Architektur-Dok** | ✅ Aktiv | `docs/dev/adr_001_event_bridge.md` + `milestone_01_event_engine.md` |

---

## Komplett-Befehl (alles aktiv, ein Befehl)

```bash
# Abhängigkeiten (für echtes Fenster + Audio + Ruby + OpenGL)
sudo apt install -y libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
  libgl1-mesa-dev libglu1-mesa-dev mesa-common-dev ruby ruby-dev rake bison g++ xvfb \
  libasound2-dev libpulse-dev libglfw3-dev

# Konfigurieren (ALLES AN)
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DAETHER_BUILD_EDITOR=ON \
  -DAETHER_BUILD_RUNTIME=ON \
  -DAETHER_BUILD_TESTS=ON \
  -DAETHER_WITH_GLFW=ON \
  -DAETHER_WITH_OPENGL=ON \
  -DAETHER_WITH_MINIAUDIO=ON \
  -DAETHER_WITH_STB=ON \
  -DAETHER_WITH_MRUBY=ON

# Bauen (jetzt inkl. event_bridge.cpp + bootstrap.cpp + Editor)
cmake --build build -j$(nproc)

# Testen (Headless-Smoke + Milestone 01)
ctest --test-dir build --output-on-failure
./build/test_event_bridge_smoke  # wenn kompiliert

# Lauf (echtes Fenster, wenn GLFW/ImGui verlinkt)
./build/bin/AetherEditor --gui --project samples/demo_project
./build/bin/Game --project samples/demo_project --max-frames 60
```

---

## Workflow-Datei (manuell / für GitHub UI)

- Lokal: `.github/workflows/ci-build.yml`
- Dokumentiert: `docs/dev/github-workflows/ci-build-active.yml`
- Inhalt: Original + `Milestone 01 – EventBridge Smoke Test` (Headless & Null backends)

---

*Autor: Senior Software Architect / Lead Engine Developer*  
*Datum: 2026-07-31*  
*Branch: arena/019fb6ee-game-engine-2*  
*Commit: 8e43d12 (Workflow), aa234c8 (Build-Script), 7edff18 (CMake), 1ba80a9 (Milestone 01)*
