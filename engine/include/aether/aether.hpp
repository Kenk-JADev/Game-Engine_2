/**
 * @file aether.hpp
 * @brief Umbrella-Header der Aether Engine.
 *
 * Bindet die stabilen öffentlichen Teilmodule ein.
 * Für produktionsnahen Code bevorzugen Sie gezielte Includes,
 * um Compile-Zeiten gering zu halten.
 *
 * @copyright AetherRPG Maker / Aether Engine
 */
#pragma once

#if defined(__has_include)
#  if __has_include(<aether/version.hpp>)
#    include <aether/version.hpp>
#  endif
#endif

#ifndef AETHER_VERSION_MAJOR
#  define AETHER_VERSION_MAJOR 0
#  define AETHER_VERSION_MINOR 1
#  define AETHER_VERSION_PATCH 0
#endif

#ifndef AETHER_VERSION_STRING
#  define AETHER_VERSION_STRING "0.1.0-dev"
#endif

#include <aether/core/core.hpp>
#include <aether/window/window_module.hpp>
#include <aether/render/render_module.hpp>
#include <aether/input/input_module.hpp>
#include <aether/audio/audio_module.hpp>
#include <aether/res/resource_module.hpp>
#include <aether/ruby/ruby_module.hpp>
#include <aether/game/event_system.hpp>
#include <aether/game/database.hpp>
#include <aether/game/player.hpp>
#include <aether/game/weather.hpp>
#include <aether/game/map_loader.hpp>
#include <aether/game/inventory.hpp>
#include <aether/game/save_system.hpp>
#include <aether/game/scene_stack.hpp>
#include <aether/game/shop.hpp>
#include <aether/anim/animation.hpp>
#include <aether/plugin/plugin_loader.hpp>
#include <aether/phys/collision.hpp>
#include <aether/nav/pathfinding.hpp>
#include <aether/scene/scene.hpp>

namespace aether {

/**
 * @brief Liefert die Engine-Versionszeichenkette.
 */
[[nodiscard]] inline const char* version() noexcept {
    return AETHER_VERSION_STRING;
}

} // namespace aether
