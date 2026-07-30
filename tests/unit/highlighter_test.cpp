/**
 * @file highlighter_test.cpp
 */
#include "../../editor/src/scripts/script_highlighter.hpp"

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
    auto t = tokenize_ruby_line("def boot; Audio.bgm_play(\"Theme\", 80) # hi");
    CHECK(!t.empty());
    bool has_kw = false, has_str = false, has_cmt = false;
    for (const auto& tk : t) {
        if (tk.kind == ScriptTokenKind::Keyword && tk.text == "def") has_kw = true;
        if (tk.kind == ScriptTokenKind::String) has_str = true;
        if (tk.kind == ScriptTokenKind::Comment) has_cmt = true;
    }
    CHECK(has_kw);
    CHECK(has_str);
    CHECK(has_cmt);

    auto comps = ruby_api_completions("Audio");
    CHECK(!comps.empty());
    CHECK(!ruby_api_doc_lines().empty());

    if (g_failures == 0) {
        std::puts("OK: highlighter_test passed");
        return 0;
    }
    std::fprintf(stderr, "FAIL: %d\n", g_failures);
    return 1;
}
