# Continuous Integration (GitHub Actions)

## Workflows

Die kanonischen YAML-Dateien liegen unter
[`docs/dev/github-workflows/`](github-workflows/) und müssen **einmalig** nach
`.github/workflows/` kopiert werden (siehe dortige README – die CI-App darf
Workflow-Dateien nicht selbst pushen).

| Datei | Trigger | Zweck |
|-------|---------|--------|
| `ci.yml` → `.github/workflows/ci.yml` | push / PR / manuell | Build + Tests Linux/Windows/macOS |
| `release.yml` → `.github/workflows/release.yml` | Tag `v*` / manuell | Release-Pakete + GitHub Release |

## Linux-Job (voll)

Installiert u. a.:

- `build-essential`, `cmake`, `ninja-build`
- X11/GL-Devs → **GLFW + OpenGL**
- `ruby`, `ruby-dev`, `bison` → **mruby** (`-DAETHER_WITH_MRUBY=ON`)
- `xvfb` → GUI-Smoke ohne physisches Display

Schritte:

1. `cmake -S . -B build …`
2. `cmake --build build`
3. `ctest --test-dir build --output-on-failure`
4. Xvfb-Smoke für `Game` und `AetherEditor`
5. Upload der Release-Binaries (nur Release-Matrix)

## Linux-Headless-Job

Baut bewusst **ohne** GLFW/OpenGL/mruby, damit Null-Backends nie regressieren.

## Windows-Job

Visual Studio 2022 x64, GLFW/OpenGL/miniaudio, mruby aus (kein Rake-Setup default).

## mruby in CI

```text
-DAETHER_WITH_MRUBY=ON
```

CMake:

1. Sucht Host-`ruby` / `rake`
2. Nutzt `third_party/mruby-src` oder FetchContent (`mruby` 3.3.0)
3. Baut via `ExternalProject` + `cmake/mruby_build_config.rb`
4. Linkt `libmruby.a` an `aether_engine`
5. Kompiliert `engine/src/ruby/mruby_vm.cpp`

Ohne Ruby fällt CI nicht um: Option bleibt an, CMake loggt „Stub only“.

## Lokal wie CI

```bash
sudo apt install build-essential cmake ninja-build \
  libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
  libgl1-mesa-dev ruby ruby-dev bison libasound2-dev xvfb

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DAETHER_WITH_GLFW=ON -DAETHER_WITH_OPENGL=ON \
  -DAETHER_WITH_MINIAUDIO=ON -DAETHER_WITH_STB=ON \
  -DAETHER_WITH_MRUBY=ON -DAETHER_BUILD_TESTS=ON

cmake --build build
ctest --test-dir build --output-on-failure
xvfb-run -a ./build/bin/AetherEditor --gui --new /tmp/g --max-frames 10
```

## Status-Badge (README)

```markdown
[![CI](https://github.com/Kenk-JADev/Game-Engine_2/actions/workflows/ci.yml/badge.svg)](https://github.com/Kenk-JADev/Game-Engine_2/actions/workflows/ci.yml)
```
