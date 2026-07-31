/**
 * @file ruby_language.hpp
 * @brief Ruby-Sprachdefinition für den ImGuiColorTextEdit-Editor.
 *
 * Eigenständige Definition (die Bibliothek bringt kein Ruby mit):
 * Keywords, Symbole, Strings, Kommentare (#), Zahlen und die
 * Aether-Engine-API-Module als Known-Identifier.
 */
#pragma once

#include "TextEditor.h"

namespace aether::editor {

/**
 * @brief Liefert die (statische) Ruby-LanguageDefinition.
 */
[[nodiscard]] const TextEditor::LanguageDefinition& ruby_language_definition();

} // namespace aether::editor
