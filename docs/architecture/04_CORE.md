# Modul 4 – Core

**Namespace:** `aether::core`  
**Status:** implementiert

---

## 1. Architektur

Das Core-Modul ist die unterste Schicht der Engine. Es stellt Infrastruktur bereit, die **alle** weiteren Subsysteme nutzen:

| Komponente | Datei | Aufgabe |
|------------|-------|---------|
| Types | `types.hpp` | `u8…f64`, `Result<T>`, `Error`, IDs |
| Assert | `assert.hpp/.cpp` | `AETHER_ASSERT`, `AETHER_UNREACHABLE` |
| Logger | `logger.hpp/.cpp` | Thread-sichere Sinks (Console, File) |
| Time | `time.hpp/.cpp` | Delta, Fixed-Step, ScopedTimer |
| Config | `config.hpp/.cpp` | `EngineConfig` JSON laden/speichern |
| ThreadPool | `thread_pool.hpp/.cpp` | Worker für I/O / Background |
| EventBus | `event_bus.hpp/.cpp` | Typsichere Engine-Events |
| EngineContext | `engine_context.hpp/.cpp` | Besitz & Lebenszyklus der Dienste |

```
EngineContext
├── Logger (+ ConsoleSink / FileSink)
├── TimeSystem
├── ThreadPool
└── EventBus
```

**Keine globalen Subsystem-Objekte.** Es gibt lediglich:

- `EngineContext::active()` – nicht-besitzender Zeiger auf den laufenden Context
- `active_logger()` – für Bequemlichkeits-Makros (`AETHER_LOG_*`)

Beide werden von `EngineContext::create` / `start` gesetzt und bei Shutdown/Destruktor geleert.

---

## 2. Dateistruktur

```
engine/include/aether/core/
  core.hpp              # Sammel-Include
  types.hpp
  assert.hpp
  logger_fwd.hpp
  logger.hpp
  time.hpp
  config.hpp
  thread_pool.hpp
  event_bus.hpp
  engine_context.hpp

engine/src/core/
  logger.cpp
  assert.cpp
  time.cpp
  config.cpp
  thread_pool.cpp
  event_bus.cpp
  engine_context.cpp
  version.cpp
```

---

## 3. Nutzung

```cpp
#include <aether/core/core.hpp>

using namespace aether::core;

int main() {
    EngineConfig cfg;
    cfg.mode = AppMode::Headless;
    cfg.log.level = "debug";

    auto ctx = EngineContext::create(std::move(cfg));
    ctx->start();

    while (ctx->pump_frame()) {
        ctx->time().drain_fixed_steps([](f64 dt) {
            // feste Simulation
            (void)dt;
        });
        // render…
        if (ctx->time().frame_count() > 3) break;
    }

    ctx->shutdown();
}
```

### Logger

```cpp
AETHER_LOG_INFO("MySystem", "Hello");
log_warn("MySystem", "Something odd");
```

Format:

```
[2026-07-30 12:00:00.123] [1401…] [INFO] [MySystem] Hello
```

### Config JSON (Beispiel)

```json
{
  "mode": "runtime",
  "graphics": { "title": "My Game", "width": 1280, "height": 720, "vsync": true, "frame_rate": 60 },
  "audio": { "bgm_volume": 80, "bgs_volume": 64, "me_volume": 80, "se_volume": 80 },
  "paths": { "assets_dir": "assets/engine", "logs_dir": "logs", "project_dir": "." },
  "log": { "console": true, "file": true, "level": "info" },
  "worker_threads": 0,
  "enable_ruby": true
}
```

### ThreadPool

```cpp
auto future = ctx->thread_pool().submit([] {
    return load_something();
});
// … später:
auto value = future.get();
```

**Wichtig:** Keine OpenGL-Aufrufe aus Worker-Threads (Kontext liegt auf dem Main-Thread).

### EventBus

```cpp
ctx->events().subscribe<FrameBeginEvent>([](const FrameBeginEvent& e) {
    // …
});
ctx->events().publish(FrameBeginEvent{dt, frame});
```

---

## 4. Designentscheidungen

1. **`Result<T>` statt Exceptions im Hot-Path** – Exceptions nur an Boot-Grenzen.
2. **Fixed Timestep** mit Accumulator und Spiral-of-Death-Cap (max. 8 Steps).
3. **RAII** für ThreadPool (join im Destruktor) und FileSink.
4. **SOLID:** Logger-Sinks über `ILogSink`; Context injiziert Dienste.
5. **C++20**, keine globalen veränderlichen Subsysteme.

---

## 5. Tests

```
tests/unit/core_test.cpp   – Result, Time, Config, Pool, Bus, Context
tests/unit/smoke_test.cpp  – Version-Link
```

```bash
cmake --build build -j
ctest --test-dir build --output-on-failure
```

---

## 6. Nächstes Modul

**Fenster (Window)** – GLFW-Fenster, GL-Context-Vorbereitung, VSync, Fullscreen, Resize-Events an den EventBus.

---

*Modul 4 abgeschlossen.*
