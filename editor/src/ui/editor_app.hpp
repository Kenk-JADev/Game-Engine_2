/**
 * @file editor_app.hpp
 * @brief AetherEditor Hauptanwendung – RPG-Maker-Bedienkonzept.
 *
 * Bereiche: Projekt | Karte | Datenbank | Events | Skripte | Testspiel | Export
 * Keine Component-Listen, keine Collider-Konfiguration.
 */
#pragma once

#include <aether/aether.hpp>
#include <aether/shared/project_descriptor.hpp>

#include "scripts/script_debugger.hpp"
#include "ui/undo_stack.hpp"

#include <memory>
#include <string>
#include <vector>

namespace aether::editor {

enum class EditorTab {
    Project = 0,
    Map,
    Database,
    Events,
    Scripts,
    TestPlay,
    Export,
    Count,
};

struct EditorAppConfig {
    bool headless = false;
    bool force_null_window = false;
    std::filesystem::path project_path;
    std::filesystem::path create_project;
    bool auto_testplay = false;
    int max_frames = -1; ///< headless smoke
};

/**
 * @brief Editor-Laufzeitzustand.
 */
class EditorApp : public aether::NonMovable {
public:
    explicit EditorApp(EditorAppConfig cfg);
    ~EditorApp();

    int run();

private:
    bool boot();
    void shutdown();
    void main_loop();
    void draw_ui();
    void draw_menu_bar();
    void draw_tab_bar();
    void draw_project_tab();
    void draw_map_tab();
    void draw_database_tab();
    void draw_events_tab();
    void draw_scripts_tab();
    void draw_testplay_tab();
    void draw_export_tab();
    void draw_status_bar();

    void open_project(const std::filesystem::path& path);
    void create_project(const std::filesystem::path& path);
    void save_project();
    void ensure_map_scene();
    void place_palette_object(const char* kind);
    void run_testplay_smoke();
    void do_export();
    void load_script_buffer();
    void save_script_buffer();
    void reload_scripts();
    void push_undo(std::string label);
    void do_undo();
    void do_redo();

    bool init_imgui();
    void shutdown_imgui();
    void begin_imgui_frame();
    void end_imgui_frame();

    EditorAppConfig cfg_;
    std::unique_ptr<core::EngineContext> ctx_;
    std::unique_ptr<window::Window> window_;
    std::unique_ptr<render::Renderer> renderer_;
    std::unique_ptr<input::InputManager> input_;
    std::unique_ptr<audio::AudioEngine> audio_;
    std::unique_ptr<res::ResourceManager> resources_;
    std::unique_ptr<ruby::RubyVM> ruby_;
    std::unique_ptr<scene::Scene> map_scene_;

    shared::ProjectDescriptor project_{};
    game::Database database_{};
    bool project_open_ = false;
    bool running_ = false;
    bool imgui_ready_ = false;

    EditorTab tab_ = EditorTab::Project;
    std::string status_message_ = "Ready";
    char project_path_buf_[512]{};
    char new_project_buf_[512]{};
    char export_path_buf_[512]{};

    // Map editor
    render::Camera map_camera_{};
    EntityId selected_id_ = kInvalidEntity;
    int palette_index_ = 0;
    UndoStack undo_;

    // Database editor
    int db_subtab_ = 0; // 0 actors 1 enemies 2 items 3 skills 4 system
    int db_selected_ = 0;

    // Events
    int event_obj_index_ = -1;
    int event_page_index_ = 0;
    int event_cmd_index_ = -1;

    // Scripts
    std::string script_path_;
    std::vector<char> script_buffer_;
    std::string script_output_;
    char script_complete_prefix_[64]{};
    ScriptDebugger debugger_;
    int bp_line_input_ = 1;

    // Stats
    u64 frame_ = 0;
};

[[nodiscard]] const char* tab_name(EditorTab t) noexcept;

} // namespace aether::editor
