/**
 * @file editor_app.cpp
 * @brief ImGui-basierter Editor mit RPG-Maker-Tabs.
 */
#include "editor_app.hpp"
#include "export/exporter.hpp"
#include "scripts/script_highlighter.hpp"

#include <cmath>
#include <cstring>
#include <fstream>
#include <sstream>

#if defined(AETHER_WITH_IMGUI)
#  include <imgui.h>
#  if defined(AETHER_WITH_GLFW) && defined(AETHER_WITH_OPENGL)
#    include <imgui_impl_glfw.h>
#    include <imgui_impl_opengl3.h>
#    include <GLFW/glfw3.h>
#    include <glad/glad.h>
#  endif
#endif

namespace aether::editor {
namespace {

bool write_text_file(const std::filesystem::path& p, const std::string& text) {
    std::ofstream out(p);
    if (!out) return false;
    out << text;
    return true;
}

std::string read_text_file(const std::filesystem::path& p) {
    std::ifstream in(p);
    if (!in) return {};
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

} // namespace

const char* tab_name(EditorTab t) noexcept {
    switch (t) {
    case EditorTab::Project: return "Projekt";
    case EditorTab::Map: return "Karte";
    case EditorTab::Database: return "Datenbank";
    case EditorTab::Events: return "Events";
    case EditorTab::Scripts: return "Skripte";
    case EditorTab::TestPlay: return "Testspiel";
    case EditorTab::Export: return "Export";
    default: return "?";
    }
}

EditorApp::EditorApp(EditorAppConfig cfg) : cfg_(std::move(cfg)) {
    if (!cfg_.project_path.empty()) {
        std::snprintf(project_path_buf_, sizeof(project_path_buf_), "%s",
                      cfg_.project_path.string().c_str());
    }
    std::snprintf(export_path_buf_, sizeof(export_path_buf_), "%s", "export/MyGame");
}

EditorApp::~EditorApp() {
    shutdown();
}

int EditorApp::run() {
    if (!boot()) {
        return 1;
    }
    if (!cfg_.create_project.empty()) {
        create_project(cfg_.create_project);
    }
    if (!cfg_.project_path.empty()) {
        open_project(cfg_.project_path);
    }
    if (cfg_.auto_testplay && project_open_) {
        run_testplay_smoke();
        if (cfg_.headless) {
            shutdown();
            return 0;
        }
    }
    main_loop();
    shutdown();
    return 0;
}

bool EditorApp::boot() {
    core::EngineConfig ecfg;
    ecfg.mode = core::AppMode::Editor;
    ecfg.graphics.title = "AetherRPG Editor";
    ecfg.graphics.width = 1440;
    ecfg.graphics.height = 900;
    ecfg.log.console = true;
    ecfg.log.file = false;
    ecfg.log.level = "info";
    if (cfg_.headless || cfg_.force_null_window) {
        // keep window null
    }

    ctx_ = core::EngineContext::create(std::move(ecfg));
    ctx_->start();

    window::WindowDesc wd = window::Window::desc_from_graphics(ctx_->config().graphics);
    wd.title = "AetherRPG Editor";
    std::optional<window::WindowBackend> force;
    if (cfg_.headless || cfg_.force_null_window) {
        force = window::WindowBackend::Null;
    }
    window_ = window::Window::create(wd, &ctx_->events(), force);
    if (!window_) {
        core::log_error("Editor", "Window create failed");
        return false;
    }

    render::RendererDesc rd;
    if (window_->backend() == window::WindowBackend::Glfw) {
        rd.backend = render::RendererBackend::OpenGL;
    } else {
        rd.backend = render::RendererBackend::Null;
    }
    rd.clear_color = render::Color{0.12f, 0.13f, 0.16f, 1.0f};
    renderer_ = render::Renderer::create(rd, window_.get());

    input_ = std::make_unique<input::InputManager>(&ctx_->events());
    input_->register_default_rpg_actions();
    input_->attach_window(window_.get());

    audio_ = audio::AudioEngine::create(
#if defined(AETHER_WITH_MINIAUDIO)
        cfg_.headless ? audio::AudioBackend::Null : audio::AudioBackend::MiniAudio,
#else
        audio::AudioBackend::Null,
#endif
        ctx_->config().audio);

    resources_ = std::make_unique<res::ResourceManager>(&ctx_->thread_pool());
    ruby_ = ruby::RubyVM::create();
    ruby_->define_engine_api();

    map_camera_.set_perspective(45.0f, 16.0f / 9.0f, 0.1f, 500.0f);
    map_camera_.look_at({8, 12, 16}, {0, 0, 0}, {0, 1, 0});

    imgui_ready_ = init_imgui();
    running_ = true;

    ctx_->events().subscribe<window::WindowCloseEvent>(
        [this](const window::WindowCloseEvent&) { running_ = false; });

    status_message_ = std::string("AetherEditor ") + aether::version();
    core::log_info("Editor", status_message_);
    return true;
}

bool EditorApp::init_imgui() {
#if defined(AETHER_WITH_IMGUI) && defined(AETHER_WITH_GLFW) && defined(AETHER_WITH_OPENGL)
    if (window_->backend() != window::WindowBackend::Glfw) {
        core::log_info("Editor", "ImGui skipped (no GLFW/GL window) – using fallback UI log");
        return false;
    }
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    auto* glfw = static_cast<GLFWwindow*>(window_->native_handle());
    ImGui_ImplGlfw_InitForOpenGL(glfw, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    core::log_info("Editor", "ImGui initialized");
    return true;
#else
    core::log_info("Editor", "ImGui not linked – headless/CLI UI mode");
    return false;
#endif
}

void EditorApp::shutdown_imgui() {
#if defined(AETHER_WITH_IMGUI) && defined(AETHER_WITH_GLFW) && defined(AETHER_WITH_OPENGL)
    if (!imgui_ready_) return;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    imgui_ready_ = false;
#endif
}

void EditorApp::begin_imgui_frame() {
#if defined(AETHER_WITH_IMGUI) && defined(AETHER_WITH_GLFW) && defined(AETHER_WITH_OPENGL)
    if (!imgui_ready_) return;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
#endif
}

void EditorApp::end_imgui_frame() {
#if defined(AETHER_WITH_IMGUI) && defined(AETHER_WITH_GLFW) && defined(AETHER_WITH_OPENGL)
    if (!imgui_ready_) return;
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif
}

void EditorApp::shutdown() {
    if (!ctx_) return;
    shutdown_imgui();
    map_scene_.reset();
    ruby_.reset();
    resources_.reset();
    audio_.reset();
    input_.reset();
    renderer_.reset();
    if (window_) {
        window_.reset();
        window::WindowSystem::terminate();
    }
    ctx_->shutdown();
    ctx_.reset();
}

void EditorApp::main_loop() {
    while (running_) {
        window_->poll_events();
        if (window_->should_close()) break;

        input_->begin_frame();
        ctx_->pump_frame();
        audio_->update(ctx_->time().delta_seconds());

        // Render map preview under UI
        if (renderer_ && map_scene_) {
            std::vector<render::Renderable> items;
            map_scene_->collect_renderables(items);
            renderer_->begin_frame();
            renderer_->draw(map_camera_, items);
            renderer_->end_frame();
        } else if (renderer_) {
            renderer_->begin_frame();
            renderer_->end_frame();
        }

        if (imgui_ready_) {
            begin_imgui_frame();
            draw_ui();
            end_imgui_frame();
        } else if (cfg_.headless) {
            // headless: limited frames
            if (cfg_.max_frames > 0 && static_cast<int>(frame_) + 1 >= cfg_.max_frames) {
                running_ = false;
            }
        }

        window_->swap_buffers();
        input_->end_frame();
        ++frame_;

        if (cfg_.max_frames > 0 && static_cast<int>(frame_) >= cfg_.max_frames) {
            running_ = false;
        }
    }
}

void EditorApp::draw_ui() {
#if defined(AETHER_WITH_IMGUI)
    if (!imgui_ready_) return;
    draw_menu_bar();

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoBringToFrontOnFocus |
                             ImGuiWindowFlags_NoNavFocus;

    ImGui::Begin("AetherMain", nullptr, flags);
    draw_tab_bar();
    ImGui::Separator();

    switch (tab_) {
    case EditorTab::Project: draw_project_tab(); break;
    case EditorTab::Map: draw_map_tab(); break;
    case EditorTab::Database: draw_database_tab(); break;
    case EditorTab::Events: draw_events_tab(); break;
    case EditorTab::Scripts: draw_scripts_tab(); break;
    case EditorTab::TestPlay: draw_testplay_tab(); break;
    case EditorTab::Export: draw_export_tab(); break;
    default: break;
    }

    ImGui::Separator();
    draw_status_bar();
    ImGui::End();
#endif
}

void EditorApp::draw_menu_bar() {
#if defined(AETHER_WITH_IMGUI)
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Projekt")) {
            if (ImGui::MenuItem("Neu…")) tab_ = EditorTab::Project;
            if (ImGui::MenuItem("Öffnen…")) tab_ = EditorTab::Project;
            if (ImGui::MenuItem("Speichern", "Ctrl+S", false, project_open_)) save_project();
            ImGui::Separator();
            if (ImGui::MenuItem("Rückgängig", "Ctrl+Z", false, project_open_ && undo_.can_undo()))
                do_undo();
            if (ImGui::MenuItem("Wiederholen", "Ctrl+Y", false, project_open_ && undo_.can_redo()))
                do_redo();
            ImGui::Separator();
            if (ImGui::MenuItem("Beenden")) running_ = false;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Ansicht")) {
            for (int i = 0; i < static_cast<int>(EditorTab::Count); ++i) {
                auto t = static_cast<EditorTab>(i);
                if (ImGui::MenuItem(tab_name(t), nullptr, tab_ == t)) tab_ = t;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Spiel")) {
            if (ImGui::MenuItem("Testspiel", "F5", false, project_open_)) {
                tab_ = EditorTab::TestPlay;
                run_testplay_smoke();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
#endif
}

void EditorApp::draw_tab_bar() {
#if defined(AETHER_WITH_IMGUI)
    for (int i = 0; i < static_cast<int>(EditorTab::Count); ++i) {
        auto t = static_cast<EditorTab>(i);
        if (i > 0) ImGui::SameLine();
        const bool selected = (tab_ == t);
        if (selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.45f, 0.75f, 1));
        }
        if (ImGui::Button(tab_name(t))) tab_ = t;
        if (selected) ImGui::PopStyleColor();
    }
#endif
}

void EditorApp::draw_project_tab() {
#if defined(AETHER_WITH_IMGUI)
    ImGui::TextUnformatted("Projektverwaltung");
    ImGui::TextWrapped(
        "Kein Unity/Unreal-Workflow: Sie arbeiten mit Projekt, Karte, Datenbank, "
        "Events und Skripten – die Engine richtet Kollision und Navigation automatisch ein.");

    ImGui::Separator();
    ImGui::InputText("Projektordner", project_path_buf_, sizeof(project_path_buf_));
    if (ImGui::Button("Öffnen")) {
        open_project(project_path_buf_);
    }
    ImGui::SameLine();
    if (ImGui::Button("Speichern") && project_open_) {
        save_project();
    }

    ImGui::Separator();
    ImGui::InputText("Neues Projekt", new_project_buf_, sizeof(new_project_buf_));
    if (ImGui::Button("Anlegen")) {
        if (new_project_buf_[0] != '\0') create_project(new_project_buf_);
    }

    if (project_open_) {
        ImGui::Separator();
        ImGui::Text("Geöffnet: %s  (v%s)", project_.name.c_str(), project_.version.c_str());
        char title[256];
        std::snprintf(title, sizeof(title), "%s", project_.graphics.title.c_str());
        if (ImGui::InputText("Spieltitel", title, sizeof(title))) {
            project_.graphics.title = title;
            project_.name = title;
        }
        ImGui::InputInt("Breite", &project_.graphics.width);
        ImGui::InputInt("Höhe", &project_.graphics.height);
        ImGui::Checkbox("Vollbild", &project_.graphics.fullscreen);
        ImGui::Checkbox("VSync", &project_.graphics.vsync);
        ImGui::InputInt("FPS", &project_.graphics.frame_rate);
        ImGui::InputInt("Start-Map", &project_.start.map_id);
        float startp[3] = {static_cast<float>(project_.start.x),
                           static_cast<float>(project_.start.y),
                           static_cast<float>(project_.start.z)};
        if (ImGui::DragFloat3("Startposition", startp, 0.1f)) {
            project_.start.x = startp[0];
            project_.start.y = startp[1];
            project_.start.z = startp[2];
        }
    }
#endif
}

void EditorApp::draw_map_tab() {
#if defined(AETHER_WITH_IMGUI)
    if (!project_open_) {
        ImGui::TextUnformatted("Bitte zuerst ein Projekt öffnen.");
        return;
    }
    ensure_map_scene();
    const char* items[] = {"Prop (Würfel)", "NPC", "Gegner", "Event", "Boden"};

    // Camera update (shared for 3D view + picking)
    {
        const float cx = std::cos(cam_yaw_) * cam_dist_;
        const float cz = std::sin(cam_yaw_) * cam_dist_;
        map_camera_.look_at({cx, cam_height_, cz}, {0, 0, 0}, {0, 1, 0});
        const ImVec2 ds = ImGui::GetIO().DisplaySize;
        if (ds.y > 1.0f) {
            map_camera_.set_perspective(45.0f, ds.x / ds.y, 0.1f, 500.0f);
        }
    }

    ImGui::BeginChild("palette", ImVec2(200, 0), true);
    ImGui::TextUnformatted("Objekt-Palette");
    ImGui::TextWrapped("Klick im Viewport platziert (Platzier-Modus) oder wählt aus.");
    ImGui::ListBox("##pal", &palette_index_, items, IM_ARRAYSIZE(items));
    ImGui::Checkbox("Platzier-Modus", &place_mode_);
    ImGui::Checkbox("Ziehen (LMB)", &drag_move_);
    if (ImGui::Button("Hier platzieren", ImVec2(-1, 0))) {
        push_undo("Platzieren");
        place_palette_object(items[palette_index_]);
    }
    ImGui::Checkbox("Grid-Snap", &grid_snap_);
    ImGui::SliderFloat("Grid", &grid_size_, 0.25f, 4.0f, "%.2f");
    if (ImGui::Button("Rückgängig", ImVec2(-1, 0))) do_undo();
    if (ImGui::Button("Wiederholen", ImVec2(-1, 0))) do_redo();
    if (ImGui::Button("Navigation backen", ImVec2(-1, 0))) {
        map_scene_->bake_navigation();
        status_message_ = "Navigation gebacken";
    }
    if (ImGui::Button("Kollision neu", ImVec2(-1, 0))) {
        map_scene_->rebuild_collision();
        status_message_ = "Kollision neu";
    }
    ImGui::Separator();
    ImGui::TextUnformatted("Mesh laden");
    ImGui::InputText("##mesh", mesh_path_buf_, sizeof(mesh_path_buf_));
    if (ImGui::Button("glTF/OBJ auf Auswahl", ImVec2(-1, 0)) && selected_id_ != kInvalidEntity &&
        mesh_path_buf_[0] && resources_) {
        auto m = resources_->load_mesh(mesh_path_buf_);
        if (m) {
            push_undo("Mesh");
            if (auto* o = map_scene_->find(selected_id_)) {
                o->mesh = m.value();
                if (renderer_) renderer_->upload_mesh(*o->mesh);
                map_scene_->rebuild_collision();
                status_message_ = std::string("Mesh: ") + mesh_path_buf_;
            }
        } else {
            status_message_ = "Mesh fehlgeschlagen";
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("hierarchy", ImVec2(200, 0), true);
    ImGui::TextUnformatted("Objekte");
    for (const auto& o : map_scene_->objects()) {
        if (ImGui::Selectable(o.name.c_str(), selected_id_ == o.id)) {
            selected_id_ = o.id;
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("viewport_col", ImVec2(0, 0), false);

    // Interactive viewport region (picks against the full-window 3D render)
    ImGui::BeginChild("viewport", ImVec2(0, -220), true, ImGuiWindowFlags_NoScrollbar);
    {
        const ImVec2 vsize = ImGui::GetContentRegionAvail();
        ImGui::InvisibleButton("##vp_btn", vsize);
        const bool hovered = ImGui::IsItemHovered();
        const ImVec2 rmin = ImGui::GetItemRectMin();
        const ImGuiIO& io = ImGui::GetIO();

        if (hovered) {
            if (io.MouseWheel != 0.0f) {
                cam_dist_ = std::clamp(cam_dist_ - io.MouseWheel * 1.5f, 4.0f, 80.0f);
            }
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
                cam_yaw_ += io.MouseDelta.x * 0.01f;
                cam_height_ = std::clamp(cam_height_ - io.MouseDelta.y * 0.05f, 1.0f, 50.0f);
            }
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                const float sx = io.MousePos.x;
                const float sy = io.MousePos.y;
                viewport_pick(sx, sy, io.DisplaySize.x, io.DisplaySize.y);
                if (!place_mode_ && selected_id_ != kInvalidEntity && drag_move_) {
                    dragging_ = true;
                    push_undo("Verschieben");
                }
            }
            if (dragging_ && ImGui::IsMouseDragging(ImGuiMouseButton_Left) &&
                selected_id_ != kInvalidEntity) {
                render::Vec3 origin, dir;
                map_camera_.screen_to_ray(io.MousePos.x, io.MousePos.y, io.DisplaySize.x,
                                          io.DisplaySize.y, origin, dir);
                render::Vec3 hit;
                if (render::Camera::ray_plane_y(origin, dir, 0.0f, hit)) {
                    if (grid_snap_) {
                        hit.x = snap_value(hit.x, grid_size_);
                        hit.z = snap_value(hit.z, grid_size_);
                    }
                    if (auto* o = map_scene_->find(selected_id_)) {
                        o->transform.position.x = hit.x;
                        o->transform.position.z = hit.z;
                        if (o->collision_id) {
                            map_scene_->collision().set_transform(o->collision_id,
                                                                  o->transform);
                        }
                    }
                }
            }
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                dragging_ = false;
            }
        }

        // Overlay help
        ImGui::SetCursorPos(ImVec2(8, 8));
        ImGui::TextColored(ImVec4(1, 1, 1, 0.85f),
                           "LMB: %s | RMB-Drag: Orbit | Wheel: Zoom",
                           place_mode_ ? "Platzieren" : "Auswählen");
        if (selected_id_ != kInvalidEntity) {
            if (auto* o = map_scene_->find(selected_id_)) {
                ImGui::Text("Auswahl: %s", o->name.c_str());
            }
        }
    }
    ImGui::EndChild();

    ImGui::BeginChild("inspector", ImVec2(0, 0), true);
    ImGui::TextUnformatted("Eigenschaften");
    ImGui::SliderFloat("Abstand", &cam_dist_, 5.0f, 60.0f);
    ImGui::SliderFloat("Höhe", &cam_height_, 2.0f, 40.0f);
    ImGui::SliderFloat("Drehung", &cam_yaw_, -3.14f, 3.14f);
    if (auto* obj = map_scene_->find(selected_id_)) {
        char name[128];
        std::snprintf(name, sizeof(name), "%s", obj->name.c_str());
        if (ImGui::InputText("Name", name, sizeof(name))) obj->name = name;
        float pos[3] = {obj->transform.position.x, obj->transform.position.y,
                        obj->transform.position.z};
        if (ImGui::DragFloat3("Position", pos, grid_snap_ ? grid_size_ : 0.1f)) {
            if (grid_snap_) {
                pos[0] = snap_value(pos[0], grid_size_);
                pos[2] = snap_value(pos[2], grid_size_);
            }
            obj->transform.position = {pos[0], pos[1], pos[2]};
            if (obj->collision_id) {
                map_scene_->collision().set_transform(obj->collision_id, obj->transform);
            }
        }
        float scl[3] = {obj->transform.scale.x, obj->transform.scale.y, obj->transform.scale.z};
        if (ImGui::DragFloat3("Größe", scl, 0.05f, 0.05f, 50.0f)) {
            obj->transform.scale = {scl[0], scl[1], scl[2]};
            if (obj->collision_id) {
                map_scene_->collision().set_transform(obj->collision_id, obj->transform);
            }
        }
        ImGui::Checkbox("Sichtbar", &obj->visible);
        ImGui::Text("Kollision auto (#%u)", obj->collision_id);
        if (obj->map_event && ImGui::Button("Events")) tab_ = EditorTab::Events;
        if (ImGui::Button("Löschen")) {
            push_undo("Löschen");
            map_scene_->remove_object(selected_id_);
            selected_id_ = kInvalidEntity;
        }
        if (ImGui::Button("Fokus")) {
            map_camera_.look_at(
                {obj->transform.position.x + cam_dist_ * 0.5f, cam_height_,
                 obj->transform.position.z + cam_dist_ * 0.5f},
                obj->transform.position, {0, 1, 0});
        }
    } else {
        ImGui::TextUnformatted("Kein Objekt – klicken oder Liste.");
    }
    if (renderer_) {
        ImGui::Text("Renderer: %s | %u drawn",
                    renderer_->backend() == render::RendererBackend::OpenGL ? "OpenGL" : "Null",
                    renderer_->stats().drawn);
    }
    ImGui::EndChild();
    ImGui::EndChild();
#endif
}

void EditorApp::draw_database_tab() {
#if defined(AETHER_WITH_IMGUI)
    if (!project_open_) {
        ImGui::TextUnformatted("Bitte zuerst ein Projekt öffnen.");
        return;
    }
    const char* subs[] = {"Helden", "Klassen", "Gegner", "Items", "Skills", "Animationen",
                          "System"};
    for (int i = 0; i < 7; ++i) {
        if (i) ImGui::SameLine();
        if (ImGui::RadioButton(subs[i], db_subtab_ == i)) {
            db_subtab_ = i;
            db_selected_ = 0;
        }
    }
    ImGui::Separator();

    auto list_and_edit = [&](auto& vec, auto&& edit_fn) {
        ImGui::BeginChild("dblist", ImVec2(200, 0), true);
        for (int i = 0; i < static_cast<int>(vec.size()); ++i) {
            if (ImGui::Selectable(vec[static_cast<usize>(i)].name.c_str(), db_selected_ == i))
                db_selected_ = i;
        }
        if (ImGui::Button("+ Neu")) {
            vec.push_back({});
            vec.back().id = static_cast<aether::u32>(vec.size());
            vec.back().name = "Neu";
            db_selected_ = static_cast<int>(vec.size()) - 1;
        }
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild("dbedit", ImVec2(0, 0), true);
        if (db_selected_ >= 0 && db_selected_ < static_cast<int>(vec.size())) {
            edit_fn(vec[static_cast<usize>(db_selected_)]);
        }
        ImGui::EndChild();
    };

    if (db_subtab_ == 0) {
        list_and_edit(database_.actors, [](game::ActorData& a) {
            char n[128];
            std::snprintf(n, sizeof(n), "%s", a.name.c_str());
            if (ImGui::InputText("Name", n, sizeof(n))) a.name = n;
            int cid = static_cast<int>(a.class_id);
            if (ImGui::InputInt("Klassen-ID", &cid)) a.class_id = static_cast<aether::u32>(cid);
            ImGui::InputInt("Max HP", &a.max_hp);
            ImGui::InputInt("Max MP", &a.max_mp);
            ImGui::InputInt("Angriff", &a.attack);
            ImGui::InputInt("Verteidigung", &a.defense);
            ImGui::InputInt("Agilität", &a.speed);
        });
    } else if (db_subtab_ == 1) {
        list_and_edit(database_.classes, [](game::ClassData& c) {
            char n[128];
            std::snprintf(n, sizeof(n), "%s", c.name.c_str());
            if (ImGui::InputText("Name", n, sizeof(n))) c.name = n;
            ImGui::InputInt("Basis HP", &c.base_hp);
            ImGui::InputInt("Basis MP", &c.base_mp);
            ImGui::InputInt("Angriff", &c.base_attack);
            ImGui::InputInt("Verteidigung", &c.base_defense);
            ImGui::InputInt("Agilität", &c.base_speed);
        });
    } else if (db_subtab_ == 2) {
        list_and_edit(database_.enemies, [](game::EnemyData& e) {
            char n[128];
            std::snprintf(n, sizeof(n), "%s", e.name.c_str());
            if (ImGui::InputText("Name", n, sizeof(n))) e.name = n;
            ImGui::InputInt("Max HP", &e.max_hp);
            ImGui::InputInt("Angriff", &e.attack);
            ImGui::InputInt("Verteidigung", &e.defense);
            ImGui::InputInt("EXP", &e.exp);
            ImGui::InputInt("Gold", &e.gold);
        });
    } else if (db_subtab_ == 3) {
        list_and_edit(database_.items, [](game::ItemData& it) {
            char n[128];
            std::snprintf(n, sizeof(n), "%s", it.name.c_str());
            if (ImGui::InputText("Name", n, sizeof(n))) it.name = n;
            ImGui::InputInt("Preis", &it.price);
            ImGui::Checkbox("Verbrauchbar", &it.consumable);
            ImGui::InputInt("HP heilen", &it.hp_recover);
            ImGui::InputInt("MP heilen", &it.mp_recover);
        });
    } else if (db_subtab_ == 4) {
        list_and_edit(database_.skills, [](game::SkillData& s) {
            char n[128];
            std::snprintf(n, sizeof(n), "%s", s.name.c_str());
            if (ImGui::InputText("Name", n, sizeof(n))) s.name = n;
            ImGui::InputInt("MP-Kosten", &s.mp_cost);
            ImGui::InputInt("Stärke", &s.power);
        });
    } else if (db_subtab_ == 5) {
        list_and_edit(database_.animations, [](game::AnimationData& a) {
            char n[128];
            std::snprintf(n, sizeof(n), "%s", a.name.c_str());
            if (ImGui::InputText("Name", n, sizeof(n))) a.name = n;
            ImGui::InputInt("Frames", &a.frames);
            ImGui::SliderFloat("Tempo", &a.speed, 0.1f, 3.0f);
        });
    } else {
        char t[256];
        std::snprintf(t, sizeof(t), "%s", database_.system.game_title.c_str());
        if (ImGui::InputText("Spieltitel", t, sizeof(t))) database_.system.game_title = t;
        int mid = static_cast<int>(database_.system.start_map_id);
        if (ImGui::InputInt("Start-Map-ID", &mid))
            database_.system.start_map_id = static_cast<aether::u32>(mid);
        char bgm[128];
        std::snprintf(bgm, sizeof(bgm), "%s", database_.system.title_bgm.c_str());
        if (ImGui::InputText("Title-BGM", bgm, sizeof(bgm))) database_.system.title_bgm = bgm;
    }

    if (ImGui::Button("Datenbank speichern")) {
        auto r = database_.save_to_directory(project_.root_dir / project_.data_path);
        status_message_ = r ? "Datenbank gespeichert" : r.error().what();
    }
#endif
}

void EditorApp::draw_events_tab() {
#if defined(AETHER_WITH_IMGUI)
    if (!project_open_) {
        ImGui::TextUnformatted("Bitte zuerst ein Projekt öffnen.");
        return;
    }
    ensure_map_scene();
    ImGui::TextWrapped(
        "Visueller Event-Editor – Befehle ohne Programmierung. Ruby nur optional über „Skript“.");

    // list event objects
    std::vector<int> event_indices;
    for (int i = 0; i < static_cast<int>(map_scene_->objects().size()); ++i) {
        if (map_scene_->objects()[static_cast<usize>(i)].map_event)
            event_indices.push_back(i);
    }
    ImGui::BeginChild("evlist", ImVec2(220, 0), true);
    for (int idx : event_indices) {
        const auto& o = map_scene_->objects()[static_cast<usize>(idx)];
        if (ImGui::Selectable(o.name.c_str(), event_obj_index_ == idx)) {
            event_obj_index_ = idx;
            event_page_index_ = 0;
            event_cmd_index_ = -1;
        }
    }
    if (ImGui::Button("Neues Event")) {
        place_palette_object("Event");
    }
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("evcmds", ImVec2(0, 0), true);
    if (event_obj_index_ >= 0 &&
        event_obj_index_ < static_cast<int>(map_scene_->objects().size())) {
        auto* obj = map_scene_->find(map_scene_->objects()[static_cast<usize>(event_obj_index_)].id);
        if (obj && obj->map_event) {
            auto& ev = *obj->map_event;
            if (ev.pages.empty()) {
                ev.pages.push_back({});
            }
            event_page_index_ =
                std::clamp(event_page_index_, 0, static_cast<int>(ev.pages.size()) - 1);
            auto& page = ev.pages[static_cast<usize>(event_page_index_)];

            ImGui::Text("Event: %s", ev.name.c_str());
            ImGui::Text("Auslöser:");
            const char* triggers[] = {"Aktionstaste", "Spieler-Touch", "Event-Touch",
                                      "Autorun", "Parallel"};
            int tr = static_cast<int>(page.trigger);
            if (ImGui::Combo("##trig", &tr, triggers, IM_ARRAYSIZE(triggers))) {
                page.trigger = static_cast<game::EventTrigger>(tr);
            }

            ImGui::Separator();
            ImGui::TextUnformatted("Befehle");
            for (int ci = 0; ci < static_cast<int>(page.commands.size()); ++ci) {
                auto& cmd = page.commands[static_cast<usize>(ci)];
                const bool sel = event_cmd_index_ == ci;
                std::string label = std::string(game::to_string(cmd.type));
                if (cmd.type == game::EventCommandType::Message) {
                    label += ": \"" + cmd.params.value("text", "") + "\"";
                }
                if (ImGui::Selectable(label.c_str(), sel)) event_cmd_index_ = ci;
            }

            if (ImGui::Button("Nachricht")) {
                page.commands.push_back(
                    {game::EventCommandType::Message, {{"text", "Hallo!"}}, {}});
            }
            ImGui::SameLine();
            if (ImGui::Button("Schalter")) {
                page.commands.push_back(
                    {game::EventCommandType::SetSwitch, {{"id", 1}, {"value", true}}, {}});
            }
            ImGui::SameLine();
            if (ImGui::Button("Variable")) {
                page.commands.push_back(
                    {game::EventCommandType::SetVariable, {{"id", 1}, {"value", 1}}, {}});
            }
            ImGui::SameLine();
            if (ImGui::Button("Teleport")) {
                page.commands.push_back({game::EventCommandType::TransferPlayer,
                                         {{"map_id", 1}, {"x", 0}, {"y", 0}, {"z", 0}},
                                         {}});
            }
            ImGui::SameLine();
            if (ImGui::Button("Skript")) {
                page.commands.push_back(
                    {game::EventCommandType::Script, {{"code", "Audio.se_play(\"Open1\")"}}, {}});
            }
            ImGui::SameLine();
            if (ImGui::Button("Warten")) {
                page.commands.push_back(
                    {game::EventCommandType::Wait, {{"frames", 30}}, {}});
            }
            ImGui::SameLine();
            if (ImGui::Button("Kampf")) {
                page.commands.push_back(
                    {game::EventCommandType::Battle, {{"enemy_id", 1}}, {}});
            }
            ImGui::SameLine();
            if (ImGui::Button("Quest")) {
                page.commands.push_back(
                    {game::EventCommandType::StartQuest, {{"id", "main_001"}}, {}});
            }
            ImGui::SameLine();
            if (ImGui::Button("Shop")) {
                page.commands.push_back({game::EventCommandType::Shop,
                                         {{"name", "Händler"}, {"items", {1}}},
                                         {}});
            }
            ImGui::SameLine();
            if (ImGui::Button("Wetter")) {
                page.commands.push_back(
                    {game::EventCommandType::SetWeather,
                     {{"type", "rain"}, {"power", 5}},
                     {}});
            }
            ImGui::SameLine();
            if (ImGui::Button("Auswahl")) {
                page.commands.push_back(
                    {game::EventCommandType::Choice,
                     {{"options", {"Ja", "Nein"}}, {"variable_id", 10}},
                     {}});
            }

            if (event_cmd_index_ >= 0 &&
                event_cmd_index_ < static_cast<int>(page.commands.size())) {
                auto& cmd = page.commands[static_cast<usize>(event_cmd_index_)];
                ImGui::Separator();
                ImGui::Text("Bearbeiten: %s", game::to_string(cmd.type));
                if (cmd.type == game::EventCommandType::Message) {
                    char buf[512];
                    std::snprintf(buf, sizeof(buf), "%s",
                                  cmd.params.value("text", "").c_str());
                    if (ImGui::InputTextMultiline("Text", buf, sizeof(buf))) {
                        cmd.params["text"] = buf;
                    }
                } else if (cmd.type == game::EventCommandType::SetSwitch) {
                    int id = cmd.params.value("id", 1);
                    bool v = cmd.params.value("value", true);
                    if (ImGui::InputInt("Schalter-Nr.", &id)) cmd.params["id"] = id;
                    if (ImGui::Checkbox("Ein", &v)) cmd.params["value"] = v;
                } else if (cmd.type == game::EventCommandType::SetVariable) {
                    int id = cmd.params.value("id", 1);
                    int v = cmd.params.value("value", 0);
                    if (ImGui::InputInt("Variable-Nr.", &id)) cmd.params["id"] = id;
                    if (ImGui::InputInt("Wert", &v)) cmd.params["value"] = v;
                } else if (cmd.type == game::EventCommandType::Script) {
                    char buf[1024];
                    std::snprintf(buf, sizeof(buf), "%s",
                                  cmd.params.value("code", "").c_str());
                    if (ImGui::InputTextMultiline("Ruby", buf, sizeof(buf))) {
                        cmd.params["code"] = buf;
                    }
                } else if (cmd.type == game::EventCommandType::Wait) {
                    int f = cmd.params.value("frames", 30);
                    if (ImGui::InputInt("Frames", &f)) cmd.params["frames"] = f;
                } else if (cmd.type == game::EventCommandType::TransferPlayer) {
                    int mid = cmd.params.value("map_id", 1);
                    float p[3] = {cmd.params.value("x", 0.0f), cmd.params.value("y", 0.0f),
                                  cmd.params.value("z", 0.0f)};
                    if (ImGui::InputInt("Map", &mid)) cmd.params["map_id"] = mid;
                    if (ImGui::DragFloat3("Ziel", p, 0.1f)) {
                        cmd.params["x"] = p[0];
                        cmd.params["y"] = p[1];
                        cmd.params["z"] = p[2];
                    }
                }
                if (ImGui::Button("Befehl löschen")) {
                    page.commands.erase(page.commands.begin() + event_cmd_index_);
                    event_cmd_index_ = -1;
                }
            }

            if (ImGui::Button("Event testen (Interpreter)")) {
                game::GameState st;
                game::EventInterpreter interp(&st);
                std::string log;
                interp.set_script_handler([&](const std::string& code) {
                    auto r = ruby_->eval(code);
                    log += r.ok ? ("OK " + r.value) : r.error;
                    log += "\n";
                });
                interp.start(page.commands);
                while (interp.update()) {
                }
                for (const auto& m : interp.messages()) log += "MSG: " + m + "\n";
                script_output_ = log.empty() ? "(keine Ausgabe)" : log;
                status_message_ = "Event ausgeführt";
            }
            if (!script_output_.empty()) {
                ImGui::TextWrapped("%s", script_output_.c_str());
            }
        }
    } else {
        ImGui::TextUnformatted("Kein Event gewählt. Legen Sie eines über die Palette an.");
    }
    ImGui::EndChild();
#endif
}

void EditorApp::draw_scripts_tab() {
#if defined(AETHER_WITH_IMGUI)
    if (!project_open_) {
        ImGui::TextUnformatted("Bitte zuerst ein Projekt öffnen.");
        return;
    }
    if (script_buffer_.empty()) load_script_buffer();

    ImGui::Text("Skript: %s", script_path_.c_str());
    ImGui::TextWrapped(
        "Ruby nur für Spiellogik. Syntax-Highlighting in der Vorschau. "
        "Autocomplete über Präfix + Liste.");

    if (ImGui::Button("Laden")) load_script_buffer();
    ImGui::SameLine();
    if (ImGui::Button("Speichern")) save_script_buffer();
    ImGui::SameLine();
    if (ImGui::Button("Ausführen / Hot-Reload")) reload_scripts();
    {
        bool en = debugger_.enabled();
        if (ImGui::Checkbox("Breakpoints", &en)) {
            debugger_.set_enabled(en);
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(70);
        ImGui::InputInt("##bpl", &bp_line_input_);
        ImGui::SameLine();
        if (ImGui::Button("BP+")) debugger_.add_breakpoint(bp_line_input_);
        ImGui::SameLine();
        if (ImGui::Button("BP-")) debugger_.remove_breakpoint(bp_line_input_);
        ImGui::SameLine();
        if (ImGui::Button("Debug Run") && ruby_) {
            save_script_buffer();
            debugger_.set_enabled(true);
            auto r = debugger_.run_file(*ruby_, script_path_);
            script_output_ =
                r.ok ? ("Debug: " + r.value + " @" + std::to_string(debugger_.current_line()))
                     : r.error;
            for (const auto& l : debugger_.log()) {
                script_output_ += "\n";
                script_output_ += l;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Step") && debugger_.paused() && ruby_) {
            auto r = debugger_.step(*ruby_);
            script_output_ =
                r.ok ? ("Step @" + std::to_string(debugger_.current_line())) : r.error;
        }
        ImGui::SameLine();
        if (ImGui::Button("Continue") && debugger_.paused() && ruby_) {
            auto r = debugger_.cont(*ruby_);
            script_output_ = r.ok ? ("Cont: " + r.value) : r.error;
        }
    }

    ImGui::BeginChild("script_edit", ImVec2(0, -220), true);
    ImGui::InputTextMultiline("##code", script_buffer_.data(), script_buffer_.size(),
                              ImVec2(-1, -1), ImGuiInputTextFlags_AllowTabInput);
    ImGui::EndChild();

    ImGui::BeginChild("script_side", ImVec2(0, 210), true);
    ImGui::Columns(3, nullptr, true);
    ImGui::TextUnformatted("Highlight-Vorschau");
    ImGui::BeginChild("hl", ImVec2(0, 160), true);
    {
        std::string src(script_buffer_.data());
        std::size_t line_start = 0;
        int lines_shown = 0;
        while (line_start <= src.size() && lines_shown < 40) {
            std::size_t line_end = src.find('\n', line_start);
            if (line_end == std::string::npos) line_end = src.size();
            const auto line = std::string_view(src).substr(line_start, line_end - line_start);
            auto tokens = tokenize_ruby_line(line);
            for (std::size_t ti = 0; ti < tokens.size(); ++ti) {
                const auto& tk = tokens[ti];
                const auto c = color_for(tk.kind);
                if (ti > 0) ImGui::SameLine(0, 0);
                ImGui::TextColored(ImVec4(c.r / 255.f, c.g / 255.f, c.b / 255.f, 1), "%s",
                                   tk.text.c_str());
            }
            if (tokens.empty()) {
                ImGui::TextUnformatted(" ");
            }
            if (line_end == src.size()) break;
            line_start = line_end + 1;
            ++lines_shown;
        }
    }
    ImGui::EndChild();

    ImGui::NextColumn();
    ImGui::TextUnformatted("Autocomplete");
    ImGui::InputText("Präfix", script_complete_prefix_, sizeof(script_complete_prefix_));
    auto comps = ruby_api_completions(script_complete_prefix_);
    ImGui::BeginChild("ac", ImVec2(0, 160), true);
    for (const auto& c : comps) {
        if (ImGui::Selectable(c.c_str())) {
            // append to buffer end (simple)
            std::string cur(script_buffer_.data());
            if (!cur.empty() && cur.back() != '\n') cur.push_back('\n');
            cur += c;
            cur.push_back('\n');
            if (cur.size() + 1 < script_buffer_.size()) {
                std::memcpy(script_buffer_.data(), cur.c_str(), cur.size() + 1);
            }
        }
    }
    ImGui::EndChild();

    ImGui::NextColumn();
    ImGui::TextUnformatted("API-Dokumentation");
    ImGui::BeginChild("apidoc", ImVec2(0, 160), true);
    for (const auto& d : ruby_api_doc_lines()) {
        ImGui::BulletText("%s", d.c_str());
    }
    ImGui::EndChild();
    ImGui::Columns(1);

    ImGui::Separator();
    ImGui::TextUnformatted("Ausgabe");
    ImGui::TextWrapped("%s", script_output_.c_str());
    ImGui::EndChild();
#endif
}

void EditorApp::draw_testplay_tab() {
#if defined(AETHER_WITH_IMGUI)
    ImGui::TextUnformatted("Testspiel");
    ImGui::TextWrapped(
        "Startet die Runtime-Logik im Debug-Kontext (Scripts, Audio-API, kurze Simulation).");
    if (!project_open_) {
        ImGui::TextUnformatted("Kein Projekt geöffnet.");
        return;
    }
    if (ImGui::Button("Testspiel starten", ImVec2(200, 40))) {
        run_testplay_smoke();
    }
    ImGui::TextWrapped("%s", script_output_.c_str());
#endif
}

void EditorApp::draw_export_tab() {
#if defined(AETHER_WITH_IMGUI)
    ImGui::TextUnformatted("Export");
    ImGui::TextWrapped("Erzeugt ein spielbares Paket mit Game-Runtime und Projektinhalten.");
    ImGui::InputText("Zielordner", export_path_buf_, sizeof(export_path_buf_));
    if (ImGui::Button("Exportieren") && project_open_) {
        do_export();
    }
#endif
}

void EditorApp::draw_status_bar() {
#if defined(AETHER_WITH_IMGUI)
    ImGui::Text("%s | Frame %llu | %s", status_message_.c_str(),
                static_cast<unsigned long long>(frame_),
                project_open_ ? project_.name.c_str() : "kein Projekt");
#endif
}

void EditorApp::push_undo(std::string label) {
    ensure_map_scene();
    if (map_scene_) {
        undo_.push(*map_scene_, std::move(label));
    }
}

void EditorApp::do_undo() {
    auto sc = undo_.undo(renderer_.get());
    if (sc) {
        map_scene_ = std::move(sc);
        selected_id_ = kInvalidEntity;
        status_message_ = std::string("Undo: ") + undo_.last_label();
    }
}

void EditorApp::do_redo() {
    auto sc = undo_.redo(renderer_.get());
    if (sc) {
        map_scene_ = std::move(sc);
        selected_id_ = kInvalidEntity;
        status_message_ = std::string("Redo: ") + undo_.last_label();
    }
}

void EditorApp::open_project(const std::filesystem::path& path) {
    std::string err;
    shared::ProjectDescriptor desc;
    if (!shared::load_project_descriptor(path, desc, &err)) {
        status_message_ = "Öffnen fehlgeschlagen: " + err;
        core::log_error("Editor", status_message_);
        return;
    }
    project_ = std::move(desc);
    project_open_ = true;
    std::snprintf(project_path_buf_, sizeof(project_path_buf_), "%s",
                  project_.root_dir.string().c_str());

    resources_->mount("data", project_.root_dir / project_.data_path);
    resources_->mount("maps", project_.root_dir / project_.maps_path);
    resources_->mount("graphics", project_.root_dir / project_.graphics_path);
    resources_->mount("audio", project_.root_dir / project_.audio_path);
    resources_->mount("scripts", project_.root_dir / "scripts");

    auto db = game::Database::load_from_directory(project_.root_dir / project_.data_path);
    database_ = db ? std::move(db.value()) : game::Database::make_default();

    map_scene_.reset();
    undo_.clear();
    ensure_map_scene();
    if (map_scene_) {
        undo_.push(*map_scene_, "Öffnen");
    }
    load_script_buffer();
    status_message_ = "Projekt geöffnet: " + project_.name;
}

void EditorApp::create_project(const std::filesystem::path& path) {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::create_directories(path, ec);
    const fs::path tmpl = fs::path("templates") / "empty_project";
    if (fs::exists(tmpl)) {
        fs::copy(tmpl, path, fs::copy_options::recursive | fs::copy_options::skip_existing, ec);
    } else {
        fs::create_directories(path / "data", ec);
        fs::create_directories(path / "maps", ec);
        fs::create_directories(path / "graphics", ec);
        fs::create_directories(path / "audio/bgm", ec);
        fs::create_directories(path / "scripts", ec);
        shared::ProjectDescriptor d;
        d.name = path.filename().string();
        d.graphics.title = d.name;
        (void)shared::save_project_descriptor(path / "project.json", d, nullptr);
        write_text_file(path / "scripts" / "main.rb",
                        "# frozen_string_literal: true\nmodule Main\n  module_function\n  def boot; end\nend\nMain.boot\n");
    }
    auto db = game::Database::make_default();
    db.system.game_title = path.filename().string();
    (void)db.save_to_directory(path / "data");
    open_project(path);
    status_message_ = "Projekt angelegt: " + path.string();
}

void EditorApp::save_project() {
    if (!project_open_) return;
    std::string err;
    if (!shared::save_project_descriptor(project_.root_dir / "project.json", project_, &err)) {
        status_message_ = err;
        return;
    }
(void)database_.save_to_directory(project_.root_dir / project_.data_path);
    if (map_scene_) {
        const auto map_path = game::map_path_for_id(
            project_.root_dir / project_.maps_path,
            static_cast<aether::u32>(project_.start.map_id));
        (void)game::save_map(map_path, *map_scene_);
    }
    save_script_buffer();
    status_message_ = "Gespeichert";
}

void EditorApp::ensure_map_scene() {
    if (map_scene_) return;
    const auto map_path = game::map_path_for_id(project_.root_dir / project_.maps_path,
                                                static_cast<aether::u32>(project_.start.map_id));
    auto loaded = game::load_map(map_path, resources_.get(), renderer_.get());
    if (loaded.scene) {
        map_scene_ = std::move(loaded.scene);
        status_message_ = loaded.message;
        if (undo_.can_undo() == false && undo_.can_redo() == false) {
            undo_.push(*map_scene_, "Basis");
        }
        return;
    }
    map_scene_ = game::create_default_map("Map 001", renderer_.get());
    undo_.push(*map_scene_, "Basis");
}

float EditorApp::snap_value(float v, float grid) noexcept {
    if (grid <= 1.0e-4f) {
        return v;
    }
    return std::round(v / grid) * grid;
}

bool EditorApp::viewport_pick(float screen_x, float screen_y, float vp_w, float vp_h) {
    ensure_map_scene();
    if (!map_scene_) {
        return false;
    }
    render::Vec3 origin, dir;
    map_camera_.screen_to_ray(screen_x, screen_y, vp_w, vp_h, origin, dir);

    if (place_mode_) {
        render::Vec3 hit;
        if (!render::Camera::ray_plane_y(origin, dir, 0.0f, hit)) {
            return false;
        }
        if (grid_snap_) {
            hit.x = snap_value(hit.x, grid_size_);
            hit.z = snap_value(hit.z, grid_size_);
        }
        hit.y = 0.0f;
        const char* items[] = {"Prop (Würfel)", "NPC", "Gegner", "Event", "Boden"};
        const char* kind = items[std::clamp(palette_index_, 0, 4)];
        push_undo("Viewport-Platzieren");
        place_at_world(hit, kind);
        return true;
    }

    const EntityId id = map_scene_->pick_ray(origin, dir);
    if (id != kInvalidEntity) {
        selected_id_ = id;
        if (auto* o = map_scene_->find(id)) {
            status_message_ = std::string("Gewählt: ") + o->name;
        }
        return true;
    }
    selected_id_ = kInvalidEntity;
    status_message_ = "Nichts getroffen";
    return false;
}

void EditorApp::place_at_world(const render::Vec3& world, const char* kind) {
    ensure_map_scene();
    render::Transform t;
    t.position = world;

    scene::ObjectType type = scene::ObjectType::Prop;
    std::string name = kind ? kind : "Prop";
    std::shared_ptr<render::Mesh> mesh = render::Mesh::create_cube(1.0f);

    if (kind && std::strstr(kind, "NPC")) {
        type = scene::ObjectType::Npc;
        name = "NPC_" + std::to_string(map_scene_->objects().size());
        t.scale = {0.6f, 1.2f, 0.6f};
        t.position.y = 0.0f;
    } else if (kind && (std::strstr(kind, "Gegner") || std::strstr(kind, "Enemy"))) {
        type = scene::ObjectType::Enemy;
        name = "Enemy_" + std::to_string(map_scene_->objects().size());
        t.scale = {0.8f, 0.8f, 0.8f};
    } else if (kind && std::strstr(kind, "Event")) {
        type = scene::ObjectType::Event;
        name = "EV" + std::to_string(map_scene_->objects().size());
        t.scale = {0.5f, 0.5f, 0.5f};
    } else if (kind && std::strstr(kind, "Boden")) {
        type = scene::ObjectType::Prop;
        name = "Boden";
        mesh = render::Mesh::create_plane(40.0f);
        t.position = {0, 0, 0};
        t.scale = {1, 1, 1};
    } else {
        t.position.y = 0.5f;
    }

    if (renderer_) {
        renderer_->upload_mesh(*mesh);
    }
    selected_id_ = map_scene_->place(type, name, mesh, t);
    if (auto* o = map_scene_->find(selected_id_)) {
        if (type == scene::ObjectType::Npc)
            o->material.albedo = render::Color{0.3f, 0.85f, 0.45f, 1};
        if (type == scene::ObjectType::Enemy)
            o->material.albedo = render::Color{1.0f, 0.35f, 0.3f, 1};
        if (type == scene::ObjectType::Event)
            o->material.albedo = render::Color{1.0f, 0.9f, 0.2f, 1};
    }
    status_message_ = "Platziert: " + name + " @ (" + std::to_string(t.position.x) + ", " +
                      std::to_string(t.position.z) + ")";
}

void EditorApp::place_palette_object(const char* kind) {
    ensure_map_scene();
    render::Vec3 p{static_cast<float>((map_scene_->objects().size() % 5) * 2), 0.0f,
                   static_cast<float>((map_scene_->objects().size() / 5) * 2)};
    if (grid_snap_) {
        p.x = snap_value(p.x, grid_size_);
        p.z = snap_value(p.z, grid_size_);
    }
    place_at_world(p, kind);
}

void EditorApp::run_testplay_smoke() {
    if (!project_open_) return;
    std::ostringstream log;
    log << "=== Testspiel ===\n";
    ruby_->define_engine_api();
    audio::AudioEngine* ap = audio_.get();
    ruby_->define_function(
        {"Audio", "bgm_play", -1, [ap](const std::vector<std::string>& args) {
             if (!args.empty()) {
                 audio::PlayParams p;
                 if (args.size() > 1) p.volume = std::stoi(args[1]);
                 ap->bgm_play(args[0], p);
             }
             return std::string("nil");
         }});

    const auto entry = project_.root_dir / project_.scripts.entry;
    auto r = ruby_->load_file(entry.string());
    log << (r.ok ? "Scripts OK\n" : ("Script error: " + r.error + "\n"));

    // short sim
    for (int i = 0; i < 5; ++i) {
        ctx_->pump_frame();
        audio_->update(1.0 / 60.0);
    }
    log << "5 Frames simuliert\n";
    if (map_scene_) {
        map_scene_->bake_navigation();
        log << "Nav-Grid: " << map_scene_->nav_grid().width << "x"
            << map_scene_->nav_grid().height << "\n";
        auto path = nav::find_path(map_scene_->nav_grid(), {0, 0, 0}, {5, 0, 5});
        log << "Pfad-Punkte: " << path.size() << "\n";
    }
    script_output_ = log.str();
    status_message_ = "Testspiel beendet";
}

void EditorApp::do_export() {
    ExportOptions opt;
    opt.project_dir = project_.root_dir;
    opt.output_dir = export_path_buf_;
    // try locate Game binary next to editor
    namespace fs = std::filesystem;
    const fs::path cand1 = fs::current_path() / "Game";
    const fs::path cand2 = fs::current_path() / "Game.exe";
    opt.game_binary = fs::exists(cand1) ? cand1 : cand2;
    auto res = export_project(opt);
    status_message_ = res.message;
    script_output_ = res.ok ? ("Export -> " + res.package_dir.string()) : res.message;
}

void EditorApp::load_script_buffer() {
    if (!project_open_) return;
    script_path_ = (project_.root_dir / project_.scripts.entry).string();
    auto txt = read_text_file(script_path_);
    if (txt.empty()) txt = "# main.rb\n";
    script_buffer_.assign(txt.begin(), txt.end());
    script_buffer_.resize(std::max<size_t>(script_buffer_.size() + 1, 64 * 1024), '\0');
}

void EditorApp::save_script_buffer() {
    if (!project_open_ || script_buffer_.empty()) return;
    write_text_file(script_path_, std::string(script_buffer_.data()));
    status_message_ = "Skript gespeichert";
}

void EditorApp::reload_scripts() {
    save_script_buffer();
    auto r = ruby_->reload_file(script_path_);
    script_output_ = r.ok ? ("Hot-Reload OK: " + r.value) : r.error;
    status_message_ = r.ok ? "Hot-Reload OK" : "Hot-Reload Fehler";
}

} // namespace aether::editor
