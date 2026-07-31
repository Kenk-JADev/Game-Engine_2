/**
 * @file ruby_language_test.cpp
 * @brief Ruby-LanguageDefinition für den Script-Editor (ImGuiColorTextEdit).
 *
 * Prüft ohne ImGui-Render-Kontext nur die Sprachdefinition:
 * Keywords, Engine-API-Identifier, Kommentar-/String-Konfiguration.
 */
#include "../../editor/src/scripts/ruby_language.hpp"

#include <cstdio>
#include <string>

using namespace aether::editor;

static int g_failures = 0;
#define CHECK(cond)                                                            \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::fprintf(stderr, "CHECK failed: %s (%s:%d)\n", #cond,         \
                         __FILE__, __LINE__);                                  \
            ++g_failures;                                                      \
        }                                                                      \
    } while (0)

int main() {
    const auto& def = ruby_language_definition();

    CHECK(def.mName == "Ruby");
    CHECK(def.mCaseSensitive);
    // Einzeiliger Kommentar ist # (kein Preprocessor)
    CHECK(def.mSingleLineComment == "#");
    CHECK(def.mPreprocChar == '\0');
    // Blockkommentar =begin/=end
    CHECK(def.mCommentStart == "=begin");
    CHECK(def.mCommentEnd == "=end");

    // Ruby-Keywords vorhanden
    CHECK(def.mKeywords.count("def") == 1);
    CHECK(def.mKeywords.count("elsif") == 1);
    CHECK(def.mKeywords.count("unless") == 1);
    CHECK(def.mKeywords.count("module_function") == 1);
    CHECK(def.mKeywords.count("yield") == 1);
    // kein C++-spezifisches Keyword im Ruby-Set
    CHECK(def.mKeywords.count("namespace") == 0);
    CHECK(def.mKeywords.count("template") == 0);

    // Engine-API-Module als Known-Identifier
    CHECK(def.mIdentifiers.count("Graphics") == 1);
    CHECK(def.mIdentifiers.count("Audio") == 1);
    CHECK(def.mIdentifiers.count("SceneManager") == 1);
    CHECK(def.mIdentifiers.count("Player") == 1);
    CHECK(def.mIdentifiers.count("NPC") == 1);
    CHECK(def.mIdentifiers.count("Enemy") == 1);
    CHECK(def.mIdentifiers.count("Camera") == 1);
    CHECK(def.mIdentifiers.count("Weather") == 1);
    CHECK(def.mIdentifiers.count("Inventory") == 1);
    CHECK(def.mIdentifiers.count("Quest") == 1);
    CHECK(def.mIdentifiers.count("Dialogue") == 1);
    CHECK(def.mIdentifiers.count("Map") == 1);
    CHECK(def.mIdentifiers.count("Game") == 1);

    // Token-Regexes vorhanden: Kommentar, String, Symbol, Zahl, Identifier
    bool has_comment = false, has_string = false, has_symbol = false;
    bool has_number = false, has_identifier = false;
    for (const auto& [rx, pal] : def.mTokenRegexStrings) {
        (void)pal;
        if (rx.find("#[^\\n]*") != std::string::npos) has_comment = true;
        if (rx.find("([^\"\\\\]|\\\\.)*") != std::string::npos) has_string = true;
        if (rx.find(":[a-zA-Z_]") != std::string::npos) has_symbol = true;
        if (rx.find("[0-9][0-9_]*") != std::string::npos) has_number = true;
        if (rx.find("[a-zA-Z_][a-zA-Z0-9_]*") != std::string::npos) has_identifier = true;
    }
    CHECK(has_comment);
    CHECK(has_string);
    CHECK(has_symbol);
    CHECK(has_number);
    CHECK(has_identifier);

    if (g_failures == 0) {
        std::puts("OK: ruby_language_test passed");
        return 0;
    }
    std::fprintf(stderr, "FAIL: %d\n", g_failures);
    return 1;
}
