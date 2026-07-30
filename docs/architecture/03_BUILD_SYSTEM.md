# Modul 3 – Build-System

**Projekt:** AetherRPG Maker  
**Tool:** CMake ≥ 3.21  
**Sprache:** C++20  
**Architektur:** x86_64 only

---

## 1. Ziele

- Ein Root-`CMakeLists.txt`, Out-of-Source-Builds
- Targets: `aether_shared`, `aether_engine`, `AetherEditor`, `Game`, Tests
- Debug / Release / RelWithDebInfo
- Einheitliche Compiler-Flags (MSVC & GCC/Clang)
- Abhängigkeiten zentral über `cmake/modules/AetherDependencies.cmake`
- Generierter Version-Header
- Install-Regeln als Grundlage für späteren Export

---

## 2. Targets

| Target | Typ | Ausgabe | Abhängigkeit |
|--------|-----|---------|--------------|
| `aether_shared` | Static Lib | `libaether_shared.a` / `.lib` | nlohmann_json |
| `aether_engine` | Static Lib | `libaether_engine.a` / `.lib` | shared, Threads |
| `AetherEditor` | Executable | `AetherEditor` | engine |
| `Game` | Executable | `Game` (+ `.exe` auf Windows) | engine |
| `aether_smoke_test` | Executable | Test | engine |

CMake-Aliase: `aether::shared`, `aether::engine`.

---

## 3. Optionen

| Option | Default | Bedeutung |
|--------|---------|-----------|
| `AETHER_BUILD_EDITOR` | ON | Editor bauen |
| `AETHER_BUILD_RUNTIME` | ON | Game-Runtime bauen |
| `AETHER_BUILD_TESTS` | ON | Tests bauen |
| `AETHER_BUILD_SAMPLES` | ON | Samples installieren |
| `AETHER_WARNINGS_AS_ERRORS` | OFF | `-Werror` / `/WX` |
| `AETHER_ENABLE_ASAN` | OFF | AddressSanitizer (nicht MSVC) |

---

## 4. Konfigurationen

| Config | Makro | Optimierung |
|--------|-------|-------------|
| Debug | `AETHER_DEBUG=1` | `-O0 -g` / `/Od /Zi` |
| Release | `AETHER_RELEASE=1`, `AETHER_DEBUG=0` | `-O2` / `/O2` |
| RelWithDebInfo | gemischt | `-O2 -g` / `/O2 /Zi` |

---

## 5. Dateien

```
CMakeLists.txt
cmake/
  aether_version.hpp.in
  modules/
    AetherCompilerFlags.cmake
    AetherDependencies.cmake
  toolchains/
    linux-x64.cmake
    windows-msvc-x64.cmake
shared/CMakeLists.txt
engine/CMakeLists.txt
editor/CMakeLists.txt
runtime/CMakeLists.txt
tests/CMakeLists.txt
```

---

## 6. Build-Anleitung

### Linux (GCC/Clang)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure
```

Binaries: `build/bin/AetherEditor`, `build/bin/Game`, `build/bin/aether_smoke_test`

### Windows (MSVC)

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug -j
ctest --test-dir build -C Debug --output-on-failure
```

### Nur Runtime

```bash
cmake -S . -B build -DAETHER_BUILD_EDITOR=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

---

## 7. Abhängigkeiten (Strategie)

| Phase | Libs |
|-------|------|
| Jetzt | nlohmann_json (FetchContent Fallback), Threads |
| Fenster/Renderer | GLFW, GLAD, glm |
| Audio | miniaudio oder OpenAL-Soft |
| Assets | stb_image, assimp |
| Scripting | mruby oder CRuby |
| Tests (später) | Catch2 |

`aether_require_glfw()` / `aether_require_glm()` sind vorbereitet und werden in den jeweiligen Modulen aufgerufen.

---

## 8. Version-Header

CMake erzeugt:

```
build/generated/aether/version.hpp
```

mit `AETHER_VERSION_MAJOR/MINOR/PATCH` und `AETHER_VERSION_STRING`.  
Include-Pfad ist für alle Libraries als `PUBLIC` gesetzt.

---

## 9. Asset-Kopie

Post-Build kopiert Editor/Runtime:

- `assets/engine` → `<exe_dir>/assets/engine`
- `assets/editor` → `<exe_dir>/assets/editor` (nur Editor)
- `templates` → `<exe_dir>/templates` (nur Editor)

---

## 10. Qualitätsregeln im Build

- C++20 enforced (`CMAKE_CXX_STANDARD_REQUIRED`)
- 32-Bit-Toolchains werden abgelehnt
- `compile_commands.json` für clangd/IDE
- Einheitliche Output-Dirs: `build/bin`, `build/lib`

---

*Nächstes Modul: Core (Logger, Types, Time, Config, ThreadPool, EngineContext)*
