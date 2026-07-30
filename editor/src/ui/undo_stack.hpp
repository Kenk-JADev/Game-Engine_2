/**
 * @file undo_stack.hpp
 * @brief Snapshot-Undo für den Karteneditor.
 */
#pragma once

#include <aether/render/renderer.hpp>
#include <aether/scene/scene.hpp>

#include <nlohmann/json.hpp>

#include <memory>
#include <string>
#include <vector>

namespace aether::editor {

class UndoStack {
public:
    void clear();
    void push(const scene::Scene& scene, std::string label = "");
    [[nodiscard]] bool can_undo() const noexcept { return index_ > 0; }
    [[nodiscard]] bool can_redo() const noexcept {
        return index_ + 1 < static_cast<int>(snaps_.size());
    }

    /**
     * @brief Stellt Snapshot her und liefert neue Scene.
     */
    [[nodiscard]] std::unique_ptr<scene::Scene> undo(render::Renderer* renderer);
    [[nodiscard]] std::unique_ptr<scene::Scene> redo(render::Renderer* renderer);

    [[nodiscard]] const std::string& last_label() const noexcept { return last_label_; }

private:
    struct Snap {
        nlohmann::json json;
        std::string label;
    };
    [[nodiscard]] std::unique_ptr<scene::Scene> restore(int idx, render::Renderer* renderer);

    std::vector<Snap> snaps_;
    int index_ = -1;
    std::string last_label_;
};

} // namespace aether::editor
