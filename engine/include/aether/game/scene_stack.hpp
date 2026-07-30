/**
 * @file scene_stack.hpp
 * @brief Spiel-Szenen (Titel, Karte, Menü, Dialog) – kein Unity-Scene-Asset.
 */
#pragma once

#include <aether/core/types.hpp>
#include <aether/input/input_manager.hpp>
#include <aether/render/color.hpp>
#include <aether/render/renderer.hpp>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace aether::game {

enum class GameSceneId {
    Title,
    Map,
    Menu,
    Dialog,
    SaveLoad,
    Battle,
    Choice,
    GameOver,
};

/**
 * @brief Kontext, den Szenen teilen (von Runtime befüllt).
 */
struct GameContext {
    input::InputManager* input = nullptr;
    render::Renderer* renderer = nullptr;
    bool request_quit = false;
    bool request_new_game = false;
    bool request_continue = false;
    int save_slot = 1;
    bool save_mode = true; ///< true=save, false=load in SaveLoad scene
    f64 delta = 0.0;
    f64 fixed_delta = 1.0 / 60.0;
    // UI messages for headless/log
    std::string status_line;
    std::vector<std::string> dialog_lines;
    int dialog_index = 0;
    bool dialog_open = false;
    // menu
    int menu_index = 0;
    int title_index = 0;
    int saveload_index = 0;
    // choice UI
    std::vector<std::string> choice_labels;
    int choice_index = 0;
    int choice_result = -1;
    bool choice_open = false;
    // battle finished ack
    bool battle_done_ack = false;
};

/**
 * @brief Eine Spielszene.
 */
class IGameScene {
public:
    virtual ~IGameScene() = default;
    [[nodiscard]] virtual GameSceneId id() const noexcept = 0;
    virtual void on_enter(GameContext& ctx) { (void)ctx; }
    virtual void on_exit(GameContext& ctx) { (void)ctx; }
    virtual void update(GameContext& ctx) = 0;
    virtual void fixed_update(GameContext& ctx) { (void)ctx; }
    /**
     * @brief Optionales UI-Overlay (nach 3D-Render).
     *        Headless: schreibt nur in ctx.status_line / dialog.
     */
    virtual void draw_overlay(GameContext& ctx) { (void)ctx; }
};

class TitleScene final : public IGameScene {
public:
    [[nodiscard]] GameSceneId id() const noexcept override {
        return GameSceneId::Title;
    }
    void on_enter(GameContext& ctx) override;
    void update(GameContext& ctx) override;
    void draw_overlay(GameContext& ctx) override;
};

class MenuScene final : public IGameScene {
public:
    [[nodiscard]] GameSceneId id() const noexcept override {
        return GameSceneId::Menu;
    }
    void on_enter(GameContext& ctx) override;
    void update(GameContext& ctx) override;
    void draw_overlay(GameContext& ctx) override;

    using Callback = std::function<void(const std::string& action)>;
    void set_callback(Callback cb) { callback_ = std::move(cb); }

private:
    Callback callback_;
    std::vector<std::string> items_{"Weiterspielen", "Items", "Speichern", "Laden",
                                    "Titel", "Beenden"};
};

class DialogScene final : public IGameScene {
public:
    [[nodiscard]] GameSceneId id() const noexcept override {
        return GameSceneId::Dialog;
    }
    void on_enter(GameContext& ctx) override;
    void update(GameContext& ctx) override;
    void draw_overlay(GameContext& ctx) override;
};

class SaveLoadScene final : public IGameScene {
public:
    [[nodiscard]] GameSceneId id() const noexcept override {
        return GameSceneId::SaveLoad;
    }
    void on_enter(GameContext& ctx) override;
    void update(GameContext& ctx) override;
    void draw_overlay(GameContext& ctx) override;

    using SlotCallback = std::function<void(int slot, bool is_save)>;
    void set_callback(SlotCallback cb) { callback_ = std::move(cb); }

private:
    SlotCallback callback_;
};

/**
 * @brief Stack: push Menu über Map, pop zurück.
 */
class GameSceneStack {
public:
    void clear();
    void push(std::unique_ptr<IGameScene> scene, GameContext& ctx);
    void pop(GameContext& ctx);
    void replace(std::unique_ptr<IGameScene> scene, GameContext& ctx);

    [[nodiscard]] IGameScene* current() noexcept;
    [[nodiscard]] const IGameScene* current() const noexcept;
    [[nodiscard]] bool empty() const noexcept { return stack_.empty(); }
    [[nodiscard]] usize size() const noexcept { return stack_.size(); }

    void update(GameContext& ctx);
    void fixed_update(GameContext& ctx);
    void draw_overlay(GameContext& ctx);

    /**
     * @brief Findet Szene von Typ im Stack (top-down).
     */
    template <typename T>
    T* find() {
        for (auto it = stack_.rbegin(); it != stack_.rend(); ++it) {
            if (auto* p = dynamic_cast<T*>(it->get())) {
                return p;
            }
        }
        return nullptr;
    }

private:
    std::vector<std::unique_ptr<IGameScene>> stack_;
};

/** @brief Map-Szene ist ein Marker – Logik bleibt in Runtime. */
class MapScene final : public IGameScene {
public:
    [[nodiscard]] GameSceneId id() const noexcept override {
        return GameSceneId::Map;
    }
    void update(GameContext& ctx) override;
    void draw_overlay(GameContext& ctx) override;
};

class ChoiceScene final : public IGameScene {
public:
    [[nodiscard]] GameSceneId id() const noexcept override {
        return GameSceneId::Choice;
    }
    void on_enter(GameContext& ctx) override;
    void update(GameContext& ctx) override;
    void draw_overlay(GameContext& ctx) override;
};

class BattleScene final : public IGameScene {
public:
    [[nodiscard]] GameSceneId id() const noexcept override {
        return GameSceneId::Battle;
    }
    void on_enter(GameContext& ctx) override;
    void update(GameContext& ctx) override;
    void draw_overlay(GameContext& ctx) override;

    using TickFn = std::function<void(GameContext&)>;
    void set_tick(TickFn fn) { tick_ = std::move(fn); }

private:
    TickFn tick_;
};

} // namespace aether::game
