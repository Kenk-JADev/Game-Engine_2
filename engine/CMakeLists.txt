# =============================================================================
# aether_engine – Kernbibliothek
# =============================================================================

option(AETHER_WITH_GLFW    "Enable GLFW window backend" ON)
option(AETHER_WITH_OPENGL  "Enable OpenGL renderer backend (requires GLFW)" OFF)

set(AETHER_ENGINE_SOURCES
    # Core
    src/core/logger.cpp
    src/core/assert.cpp
    src/core/time.cpp
    src/core/config.cpp
    src/core/thread_pool.cpp
    src/core/event_bus.cpp
    src/core/engine_context.cpp
    src/core/version.cpp
    # Window
    src/window/window.cpp
    src/window/null_window.cpp
    # Render
    src/render/camera.cpp
    src/render/mesh.cpp
    src/render/shader.cpp
    src/render/renderer.cpp
    src/render/null_renderer.cpp
    # Input
    src/input/input_manager.cpp
    # Audio
    src/audio/audio_engine.cpp
    src/audio/null_audio.cpp
    # Resources
    src/res/resource_manager.cpp
    # Ruby
    src/ruby/ruby_vm.cpp
    # Game framework
    src/game/event_system.cpp
    src/game/database.cpp
    # Plugins
    src/plugin/plugin_loader.cpp
)

set(AETHER_ENGINE_HEADERS
    include/aether/aether.hpp
    include/aether/core/core.hpp
    include/aether/core/types.hpp
    include/aether/core/assert.hpp
    include/aether/core/logger_fwd.hpp
    include/aether/core/logger.hpp
    include/aether/core/time.hpp
    include/aether/core/config.hpp
    include/aether/core/thread_pool.hpp
    include/aether/core/event_bus.hpp
    include/aether/core/engine_context.hpp
    include/aether/window/window.hpp
    include/aether/window/window_events.hpp
    include/aether/window/window_module.hpp
    include/aether/render/math.hpp
    include/aether/render/color.hpp
    include/aether/render/camera.hpp
    include/aether/render/mesh.hpp
    include/aether/render/material.hpp
    include/aether/render/shader.hpp
    include/aether/render/renderable.hpp
    include/aether/render/renderer.hpp
    include/aether/render/render_module.hpp
    include/aether/input/keys.hpp
    include/aether/input/input_events.hpp
    include/aether/input/input_manager.hpp
    include/aether/input/input_module.hpp
    include/aether/audio/audio_engine.hpp
    include/aether/audio/audio_module.hpp
    include/aether/res/resource_manager.hpp
    include/aether/res/resource_module.hpp
    include/aether/ruby/ruby_vm.hpp
    include/aether/ruby/ruby_module.hpp
    include/aether/game/event_system.hpp
    include/aether/game/database.hpp
    include/aether/plugin/plugin_loader.hpp
)

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------
aether_require_glm()
if(NOT AETHER_GLM_AVAILABLE)
    message(FATAL_ERROR "glm is required for the renderer module")
endif()

set(AETHER_GLFW_AVAILABLE FALSE)
if(AETHER_WITH_GLFW)
    aether_require_glfw()
endif()

if(AETHER_GLFW_AVAILABLE AND TARGET glfw)
    list(APPEND AETHER_ENGINE_SOURCES src/window/glfw_window.cpp)
    message(STATUS "Aether Window backend: GLFW")
else()
    set(AETHER_GLFW_AVAILABLE FALSE)
    message(STATUS "Aether Window backend: Null (headless)")
endif()

message(STATUS "Aether Renderer backend: Null (+ culling/LOD); OpenGL path optional")
message(STATUS "Aether Audio backend: Null (MiniAudio optional later)")

# -----------------------------------------------------------------------------
# Library
# -----------------------------------------------------------------------------
add_library(aether_engine STATIC
    ${AETHER_ENGINE_SOURCES}
    ${AETHER_ENGINE_HEADERS}
)
add_library(aether::engine ALIAS aether_engine)

target_include_directories(aether_engine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/generated>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(aether_engine
    PUBLIC
        aether::shared
        aether_third_party_interface
        Threads::Threads
        glm::glm
)

if(AETHER_GLFW_AVAILABLE AND TARGET glfw)
    target_link_libraries(aether_engine PUBLIC glfw)
    target_compile_definitions(aether_engine PUBLIC AETHER_WITH_GLFW=1)
    find_package(OpenGL)
    if(OpenGL_FOUND)
        target_link_libraries(aether_engine PUBLIC OpenGL::GL)
    endif()
endif()

if(AETHER_WITH_OPENGL AND AETHER_GLFW_AVAILABLE)
    target_compile_definitions(aether_engine PUBLIC AETHER_WITH_OPENGL=1)
endif()

aether_apply_common_options(aether_engine)

if(NOT MSVC)
    target_compile_options(aether_engine PRIVATE -Wno-pre-c++17-compat)
endif()

set_target_properties(aether_engine PROPERTIES
    OUTPUT_NAME "aether_engine"
    FOLDER "aether"
)

install(DIRECTORY include/aether
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp"
)

install(DIRECTORY shaders
    DESTINATION ${CMAKE_INSTALL_DATADIR}/aether/engine
    FILES_MATCHING PATTERN "*.vert" PATTERN "*.frag"
)
