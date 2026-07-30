/**
 * @file script_highlighter.hpp
 * @brief Minimales Ruby-Syntax-Highlighting für den Editor (Token → Farben).
 *
 * Kein vollständiger Parser – gut genug für Keywords, Strings, Kommentare, Zahlen.
 */
#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace aether::editor {

enum class ScriptTokenKind {
    Text,
    Keyword,
    String,
    Comment,
    Number,
    Symbol,
    Ident,
};

struct ScriptToken {
    ScriptTokenKind kind = ScriptTokenKind::Text;
    std::string text;
};

struct Rgba8 {
    unsigned char r = 220, g = 220, b = 220, a = 255;
};

[[nodiscard]] Rgba8 color_for(ScriptTokenKind k) noexcept;

/**
 * @brief Tokenisiert eine Zeile Ruby-ähnlich.
 */
[[nodiscard]] std::vector<ScriptToken> tokenize_ruby_line(std::string_view line);

/**
 * @brief API-Hilfe: bekannte Engine-Module für Autocomplete-Vorschläge.
 */
[[nodiscard]] std::vector<std::string> ruby_api_completions(std::string_view prefix);

/**
 * @brief Kurze API-Doku-Zeilen.
 */
[[nodiscard]] std::vector<std::string> ruby_api_doc_lines();

} // namespace aether::editor
