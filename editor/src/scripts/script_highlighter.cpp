/**
 * @file script_highlighter.cpp
 */
#include "script_highlighter.hpp"

#include <algorithm>
#include <cctype>
#include <unordered_set>

namespace aether::editor {
namespace {

const std::unordered_set<std::string> kKeywords = {
    "begin",  "end",    "def",    "class",  "module", "if",     "elsif",  "else",
    "unless", "while",  "until",  "for",    "do",     "return", "yield",  "break",
    "next",   "case",   "when",   "then",   "and",    "or",     "not",    "in",
    "self",   "super",  "true",   "false",  "nil",    "rescue", "ensure", "alias",
    "module_function", "attr_reader", "attr_writer", "attr_accessor", "require",
    "include", "extend", "private", "public", "protected",
};

bool is_ident_start(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_' || c == '@' || c == '$';
}
bool is_ident(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '?' || c == '!';
}

} // namespace

Rgba8 color_for(ScriptTokenKind k) noexcept {
    switch (k) {
    case ScriptTokenKind::Keyword: return {198, 120, 221, 255};
    case ScriptTokenKind::String: return {152, 195, 121, 255};
    case ScriptTokenKind::Comment: return {92, 99, 112, 255};
    case ScriptTokenKind::Number: return {209, 154, 102, 255};
    case ScriptTokenKind::Symbol: return {86, 182, 194, 255};
    case ScriptTokenKind::Ident: return {224, 108, 117, 255};
    case ScriptTokenKind::Text:
    default: return {171, 178, 191, 255};
    }
}

std::vector<ScriptToken> tokenize_ruby_line(std::string_view line) {
    std::vector<ScriptToken> out;
    std::size_t i = 0;
    const std::size_t n = line.size();
    auto emit = [&](ScriptTokenKind k, std::string t) {
        ScriptToken tok;
        tok.kind = k;
        tok.text = std::move(t);
        out.push_back(std::move(tok));
    };
    while (i < n) {
        if (line[i] == '#') {
            emit(ScriptTokenKind::Comment, std::string(line.substr(i)));
            break;
        }
        if (line[i] == '"' || line[i] == '\'') {
            const char q = line[i];
            std::size_t j = i + 1;
            while (j < n) {
                if (line[j] == '\\' && j + 1 < n) {
                    j += 2;
                    continue;
                }
                if (line[j] == q) {
                    ++j;
                    break;
                }
                ++j;
            }
            emit(ScriptTokenKind::String, std::string(line.substr(i, j - i)));
            i = j;
            continue;
        }
        if (line[i] == ':' && i + 1 < n && is_ident_start(line[i + 1])) {
            std::size_t j = i + 1;
            while (j < n && is_ident(line[j])) ++j;
            emit(ScriptTokenKind::Symbol, std::string(line.substr(i, j - i)));
            i = j;
            continue;
        }
        if (std::isdigit(static_cast<unsigned char>(line[i]))) {
            std::size_t j = i;
            while (j < n &&
                   (std::isdigit(static_cast<unsigned char>(line[j])) || line[j] == '.'))
                ++j;
            emit(ScriptTokenKind::Number, std::string(line.substr(i, j - i)));
            i = j;
            continue;
        }
        if (is_ident_start(line[i])) {
            std::size_t j = i;
            while (j < n && is_ident(line[j])) ++j;
            std::string w(line.substr(i, j - i));
            const auto kind =
                kKeywords.count(w) ? ScriptTokenKind::Keyword : ScriptTokenKind::Ident;
            emit(kind, std::move(w));
            i = j;
            continue;
        }
        std::size_t j = i + 1;
        while (j < n && !is_ident_start(line[j]) &&
               !std::isdigit(static_cast<unsigned char>(line[j])) && line[j] != '#' &&
               line[j] != '"' && line[j] != '\'' && line[j] != ':') {
            ++j;
        }
        emit(ScriptTokenKind::Text, std::string(line.substr(i, j - i)));
        i = j;
    }
    return out;
}

std::vector<std::string> ruby_api_completions(std::string_view prefix) {
    static const char* kApi[] = {
        "Graphics.frame_rate", "Graphics.width", "Graphics.height", "Graphics.update",
        "Audio.bgm_play", "Audio.bgm_stop", "Audio.bgs_play", "Audio.se_play", "Audio.me_play",
        "Input.press?", "Input.trigger?", "Input.repeat?",
        "SceneManager.goto", "SceneManager.call", "SceneManager.return",
        "Player.transfer", "Player.x", "Player.y", "Player.z",
        "NPC.find", "Enemy.spawn",
        "Camera.move_to", "Camera.shake",
        "Weather.set", "Weather.clear",
        "Inventory.gain", "Inventory.lose", "Inventory.count", "Inventory.gold",
        "Quest.start", "Quest.complete", "Quest.active?",
        "Dialogue.start", "Dialogue.choice",
        "Map.id", "Map.tint", "Map.scroll",
        "Game.switch", "Game.set_switch",
    };
    std::vector<std::string> out;
    const std::string p(prefix);
    for (const char* a : kApi) {
        std::string s(a);
        if (p.empty() || s.find(p) != std::string::npos) {
            out.push_back(std::move(s));
        }
    }
    return out;
}

std::vector<std::string> ruby_api_doc_lines() {
    return {
        "Graphics.frame_rate = 60",
        "Audio.bgm_play(\"Theme1\", 80, 100)",
        "Audio.se_play(\"Open1\", 80, 100)",
        "Input.press?(:C) / Input.trigger?(:B)",
        "Player.transfer(map_id, x, y, z, dir)",
        "Weather.set(\"rain\", 5)",
        "Inventory.gain(item_id, amount)",
        "Quest.start(\"main_001\")",
        "Dialogue.start(\"intro\")",
        "Map.tint(r, g, b, frames)",
        "Game.set_switch(id, true)",
        "NPC.find(\"elder\")",
    };
}

} // namespace aether::editor
