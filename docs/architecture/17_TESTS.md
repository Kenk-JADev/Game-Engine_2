# Modul 17 – Tests

**Status:** 22 automatisierte Tests (Unit + Runtime/Editor-Smoke)

## Unit- / Smoke-Tests

| Test | Modul |
|------|-------|
| `aether_smoke_test` | Version / Link |
| `aether_core_test` | Logger, Time, Config, Pool, Bus, Context |
| `aether_window_test` | NullWindow API + Events |
| `aether_render_test` | Camera, Frustum, LOD, Stats |
| `aether_input_test` | Actions, Edges, Mouse |
| `aether_audio_test` | BGM/SE Kanäle, Fade |
| `aether_res_test` | VFS, Cache, JSON, async |
| `aether_ruby_test` | Stub-VM, Host-API, Hot-Reload |
| `aether_project_test` | project.json Roundtrip |
| `aether_game_test` | Events, Database, Plugins |
| `aether_runtime_smoke` | Game --headless --max-frames 5 |

```bash
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Nächste Test-Ausbauten

- Catch2/GoogleTest Integration
- OpenGL-Renderer-Smoke (mit xvfb)
- Editor-Export Roundtrip
- Ruby mruby-Integrationstests
