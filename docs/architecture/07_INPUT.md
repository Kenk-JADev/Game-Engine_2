# Modul 7 – Input

**Namespace:** `aether::input`  
**Status:** implementiert

---

## 1. Architektur

| Komponente | Aufgabe |
|------------|---------|
| `Key` / `MouseButton` / `GamepadButton` | Backend-neutrale Codes |
| `InputManager` | Zustand, Edges, Action-Mapping |
| Events | `KeyEvent`, `MouseButtonEvent`, `MouseMoveEvent`, … |

**RPG-Maker-ähnliche Abfragen:**

| Methode | Ruby-Analog |
|---------|-------------|
| `is_down("confirm")` | `Input.press?(:C)` |
| `was_pressed("confirm")` | `Input.trigger?(:C)` |
| `was_released("confirm")` | `Input.repeat?` / release |

---

## 2. Default-Actions

| Action | Tasten |
|--------|--------|
| confirm | Z, Space, Enter, C, Maus-Links |
| cancel | X, Escape, Numpad0, Maus-Rechts |
| menu | Shift, V |
| up/down/left/right | Pfeile + WASD |
| page_up / page_down | Q/E, PageUp/Down |

---

## 3. Frame-Ablauf

```
window.poll_events()
input.begin_frame()
// feed_* (oder GLFW-Callbacks)
// game logic: was_pressed / is_down
input.end_frame()
```

---

## 4. Tests

`tests/unit/input_test.cpp`

---

*Nächstes Modul: Audio*
