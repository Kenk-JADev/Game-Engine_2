/**
 * @file ruby_language.cpp
 */
#include "ruby_language.hpp"

namespace aether::editor {

const TextEditor::LanguageDefinition& ruby_language_definition() {
    static const TextEditor::LanguageDefinition def = [] {
        TextEditor::LanguageDefinition d;
        d.mName = "Ruby";
        d.mCaseSensitive = true;
        d.mAutoIndentation = true;
        d.mPreprocChar = '\0'; // # ist Kommentar, kein Preprocessor
        d.mSingleLineComment = "#";
        d.mCommentStart = "=begin";
        d.mCommentEnd = "=end";

        d.mKeywords = {
            "alias",    "and",      "begin",     "break",    "case",
            "class",    "def",      "defined?", "do",       "else",
            "elsif",    "end",      "ensure",    "for",      "if",
            "in",       "module",   "next",      "nil",      "not",
            "or",       "redo",     "require",   "require_relative", "rescue",
            "retry",    "return",   "self",      "super",    "then",
            "unless",   "until",    "when",      "while",    "yield",
            "true",     "false",    "module_function", "attr_accessor",
            "attr_reader", "attr_writer", "private", "public", "protected",
        };

        // Engine-API-Module + wichtige Methoden als Known-Identifier
        d.mIdentifiers.insert({"Graphics", TextEditor::Identifier{}});
        d.mIdentifiers.insert({"Audio", TextEditor::Identifier{}});
        d.mIdentifiers.insert({"Input", TextEditor::Identifier{}});
        d.mIdentifiers.insert({"SceneManager", TextEditor::Identifier{}});
        d.mIdentifiers.insert({"Player", TextEditor::Identifier{}});
        d.mIdentifiers.insert({"NPC", TextEditor::Identifier{}});
        d.mIdentifiers.insert({"Enemy", TextEditor::Identifier{}});
        d.mIdentifiers.insert({"Camera", TextEditor::Identifier{}});
        d.mIdentifiers.insert({"Weather", TextEditor::Identifier{}});
        d.mIdentifiers.insert({"Inventory", TextEditor::Identifier{}});
        d.mIdentifiers.insert({"Quest", TextEditor::Identifier{}});
        d.mIdentifiers.insert({"Dialogue", TextEditor::Identifier{}});
        d.mIdentifiers.insert({"Map", TextEditor::Identifier{}});
        d.mIdentifiers.insert({"Game", TextEditor::Identifier{}});
        d.mIdentifiers.insert({"Main", TextEditor::Identifier{}});
        d.mIdentifiers.insert({"puts", TextEditor::Identifier{}});
        d.mIdentifiers.insert({"print", TextEditor::Identifier{}});
        d.mIdentifiers.insert({"p", TextEditor::Identifier{}});

        d.mTokenRegexStrings = {
            // Reihenfolge ist wichtig (erste Übereinstimmung gewinnt)
            {R"(=begin[\s\S]*?=end)", TextEditor::PaletteIndex::Comment},
            {R"(#[^\n]*)", TextEditor::PaletteIndex::Comment},
            {R"("([^"\\]|\\.)*")", TextEditor::PaletteIndex::String},
            {R"('([^'\\]|\\.)*')", TextEditor::PaletteIndex::String},
            {R"(:[a-zA-Z_][a-zA-Z0-9_]*[?!]?)", TextEditor::PaletteIndex::CharLiteral},
            {R"(\$[a-zA-Z_][a-zA-Z0-9_]*)", TextEditor::PaletteIndex::Identifier},
            {R"(@@?[a-zA-Z_][a-zA-Z0-9_]*)", TextEditor::PaletteIndex::Identifier},
            {R"([+-]?[0-9][0-9_]*(\.[0-9]+)?([eE][+-]?[0-9]+)?)", TextEditor::PaletteIndex::Number},
            {R"([a-zA-Z_][a-zA-Z0-9_]*[?!])", TextEditor::PaletteIndex::Identifier},
            {R"([a-zA-Z_][a-zA-Z0-9_]*)", TextEditor::PaletteIndex::Identifier},
            {R"(\s+)", TextEditor::PaletteIndex::Default},
        };
        return d;
    }();
    return def;
}

} // namespace aether::editor
