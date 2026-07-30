# Coding Standards – Aether Engine

## Sprache & Standard

- **C++20**
- Compiler: MSVC 2022+, GCC 11+, Clang 14+
- Warnungen: `/W4` bzw. `-Wall -Wextra -Wpedantic`; Warnungen als Fehler in CI

## Stil

| Element | Regel |
|---------|--------|
| Namespaces | `aether::subsystem` |
| Klassen / Structs | `PascalCase` |
| Funktionen / Methoden | `snake_case` |
| Member-Variablen | `snake_case_` (trailing underscore) |
| Konstanten | `kPascalCase` |
| Makros | `AETHER_NAME` |
| Dateien | `snake_case.hpp` / `snake_case.cpp` |

## Speicher & Lebensdauer

- RAII verpflichtend
- Bevorzugt `std::unique_ptr` für exklusiven Besitz
- `std::shared_ptr` nur bei geteilter Lebensdauer
- Keine ungeschützten globalen veränderlichen Variablen
- Engine-Dienste über `EngineContext` / Referenzen injizieren

## Header

```cpp
#pragma once

#include <aether/...>   // eigene
#include <third_party>  // extern
#include <standard>     // STL

namespace aether::core {
// ...
} // namespace aether::core
```

- Forward-Declarations wo möglich
- Keine `using namespace` in Headern

## Dokumentation

- Öffentliche API mit Doxygen (`@file`, `@brief`, `@param`, `@return`)
- Komplexe Algorithmen im `.cpp` kurz erklären

## Fehler

- Erwartbare Fehler: `std::optional` / `std::expected` (C++23 falls verfügbar) / Error-Codes
- Programmierfehler: `AETHER_ASSERT` (Debug)
- Fatale Startfehler: Exception an der Boot-Grenze fangen und loggen

## Ruby

- Nur Spiellogik
- Keine direkten Engine-Pointer an Skripte
- API-Module: `Graphics`, `Audio`, `Input`, `SceneManager`, …
