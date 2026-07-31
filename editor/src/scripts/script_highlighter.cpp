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
        "Graphics.frame_rate", "Graphics.width", "Graphics.height", "Graphics.fade_out",
        "Graphics.fade_in", "Graphics.update",
        "Audio.bgm_play", "Audio.bgm_stop", "Audio.bgs_play", "Audio.bgs_stop",
        "Audio.me_play", "Audio.me_stop", "Audio.se_play", "Audio.se_stop",
        "Audio.stop_all", "Audio.bgm_volume", "Audio.se_volume",
        "Input.press?", "Input.trigger?", "Input.released?", "Input.dir4", "Input.dir8",
        "SceneManager.goto", "SceneManager.scene",
        "Player.transfer", "Player.x", "Player.y", "Player.z", "Player.facing",
        "Player.set_position", "Player.move",
        "NPC.find", "NPC.x", "NPC.y", "NPC.z", "NPC.set_position", "NPC.say",
        "Enemy.spawn", "Enemy.count", "Enemy.kill", "Enemy.alive?",
        "Camera.move_to", "Camera.look_at", "Camera.zoom", "Camera.shake", "Camera.reset",
        "Weather.set", "Weather.clear", "Weather.type", "Weather.power",
        "Inventory.gain", "Inventory.lose", "Inventory.count", "Inventory.has?",
        "Inventory.gold", "Inventory.gold=", "Inventory.items",
        "Quest.start", "Quest.complete", "Quest.fail", "Quest.advance",
        "Quest.active?", "Quest.completed?", "Quest.list",
        "Dialogue.start", "Dialogue.text", "Dialogue.choices", "Dialogue.choice",
        "Dialogue.close",
        "Map.id", "Map.name", "Map.load", "Map.width", "Map.height", "Map.tint",
        "Game.switch", "Game.set_switch", "Game.variable", "Game.set_variable",
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
        "Graphics.fade_out(30) / Graphics.fade_in(30)",
        "Audio.bgm_play(\"Theme1\", 80, 100)",
        "Audio.se_play(\"Open1\", 80, 100)",
        "Input.press?(:C) / Input.trigger?(:B) / Input.dir4",
        "SceneManager.goto(:menu) / SceneManager.goto(\"exit\")",
        "Player.transfer(map_id, x, y, z, dir)",
        "Player.set_position(x, y, z) / Player.move(dx, dy, dz)",
        "NPC.say(\"Bob\", \"Hallo!\") / NPC.set_position(name, x, y, z)",
        "Enemy.spawn(enemy_id, x, y, z) / Enemy.kill(name)",
        "Camera.move_to(x, y, z) / Camera.zoom(fov) / Camera.shake(power, sec)",
        "Weather.set(\"rain\", 5) / Weather.clear",
        "Inventory.gain(item_id_or_name, amount) / Inventory.gold = 100",
        "Quest.start(\"main_001\") / Quest.complete(id) / Quest.active?(id)",
        "Dialogue.start(\"intro\") / Dialogue.choices(\"Ja\", \"Nein\")",
        "Map.load(map_id) / Map.id / Map.tint(r, g, b, a)",
        "Game.set_switch(id, true) / Game.set_variable(id, value)",
    };
}

} // namespace aether::editor
