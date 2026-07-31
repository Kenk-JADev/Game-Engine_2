# Modul 31 – Script-Editor als echte IDE

**Status:** implementiert

---

## 1. Ziel

Der Skript-Tab war eine einfache `InputTextMultiline` mit Highlight-Vorschau in
einer separaten Spalte. Jetzt ist er eine kleine IDE:

- **Inline-Syntax-Highlighting** direkt im Editor (keine Vorschau-Spalte mehr)
- **Breakpoints** direkt am Editor (Button „BP am Cursor“, Marker im Rand)
- **Autocomplete-Popup** beim Tippen (`Module.`-Präfix) oder Strg+Leertaste
- **Debugger-Synchronisation**: Editor-Breakpoints → `ScriptDebugger`,
  aktuelle Debug-Zeile springt in den Editor

## 2. Komponente: ImGuiColorTextEdit

- Vendored unter `third_party/ImGuiColorTextEdit/` (MIT, wie imgui/stb/miniaudio)
- Neues CMake-Target `aether_imgui_texteditor` (`TextEditor.cpp`)
- Bietet: `Render`, `SetText/GetText`, `InsertText` (ersetzt Selektion),
  `SetBreakpoints/SetLanguageDefinition`, Zeilennummern, Undo/Redo, Suche

### Ruby-Sprachdefinition (eigenständig)

Die Bibliothek bringt kein Ruby mit → `editor/src/scripts/ruby_language.cpp`:

- Keywords: `def/end/if/elsif/unless/module_function/yield/…`
- Strings `"…"` und `'…'`, Symbole `:sym`, Kommentare `#` + `=begin/=end`
- Zahlen (auch `1_000`, Exponenten), Globals `$var`, Instanzvariablen `@var`
- Engine-API-Module (`Graphics`, `Audio`, `Input`, `SceneManager`, `Player`,
  `NPC`, `Enemy`, `Camera`, `Weather`, `Inventory`, `Quest`, `Dialogue`,
  `Map`, `Game`) als **Known-Identifier** (hervorgehoben)
- `?`/`!`-Methoden wie `press?`, `active?`

## 3. Autocomplete

- Trigger: Strg+Leertaste oder aktuelle Wort-Präfix enthält `.` (z. B. `Audio.bg`)
- Kandidaten aus `ruby_api_completions(prefix)` (Editor-eigene API-Liste)
- Auswahl ersetzt das getippte Präfix (Selektion + `InsertText`)

## 4. Debuggen

| Aktion | Verhalten |
|--------|-----------|
| „BP am Cursor“ | setzt/löscht Breakpoint an der Cursorzeile (Marker im Rand) |
| „Debug-Modus“ | übernimmt Editor-Breakpoints in den `ScriptDebugger` |
| „Debug Run“ | führt das Skript zeilenweise aus, stoppt an Breakpoints |
| „Step“ / „Continue“ | Schritt / weiter; Cursor springt zur aktuellen Zeile |

Breakpoints: Editor **0-basiert** ↔ Debugger **1-basiert** (Konvertierung `+1`).

## 5. Tests

- `tests/unit/ruby_language_test.cpp` (Test #27 `aether_ruby_lang_test`):
  Ruby-Definition enthält Keywords (`def/elsif/unless/…`), keine C++-Keywords,
  alle Engine-Module, `#`-Kommentar, `=begin/=end`, Token-Regexes
- Bestehender `aether_highlighter_test` (Token-API) bleibt

---

*Nächstes Modul: Event-Editor-Ausbau*
