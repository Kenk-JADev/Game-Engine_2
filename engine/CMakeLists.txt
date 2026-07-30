# =============================================================================
# aether_engine – Kernbibliothek
# =============================================================================

option(AETHER_WITH_GLFW      "Enable GLFW window backend" ON)
option(AETHER_WITH_OPENGL    "Enable OpenGL renderer (needs GLFW+GLAD)" ON)
option(AETHER_WITH_MINIAUDIO "Enable miniaudio backend" ON)
option(AETHER_WITH_STB       "Enable stb_image textures" ON)
option(AETHER_WITH_MRUBY     "Enable mruby scripting (needs host Ruby/rake)" OFF)

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
    src/input/glfw_input.cpp
    # Audio
    src/audio/audio_engine.cpp
    src/audio/null_audio.cpp
    # Resources
    src/res/resource_manager.cpp
    # Ruby
    src/ruby/ruby_vm.cpp
    # mruby_vm.cpp optionally appended below
    # Game framework
    src/game/event_system.cpp
    src/game/database.cpp
    src/game/player.cpp
    src/game/weather.cpp
    src/game/map_loader.cpp
    src/game/inventory.cpp
    src/game/save_system.cpp
    src/game/scene_stack.cpp
    src/game/shop.cpp
    # Animation
    src/anim/animation.cpp
    # Plugins
    src/plugin/plugin_loader.cpp
    # Physics / Nav / Scene
    src/phys/collision.cpp
    src/nav/pathfinding.cpp
    src/scene/scene.cpp
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
    list(APPEND AETHER_ENGINE_SOURCES
        src/window/glfw_window.cpp
        src/window/glfw_proc.cpp
    )
    message(STATUS "Aether Window backend: GLFW")
else()
    set(AETHER_GLFW_AVAILABLE FALSE)
    message(STATUS "Aether Window backend: Null (headless)")
endif()

set(AETHER_OPENGL_AVAILABLE FALSE)
if(AETHER_WITH_OPENGL AND AETHER_GLFW_AVAILABLE AND TARGET aether_glad)
    list(APPEND AETHER_ENGINE_SOURCES src/render/gl_renderer.cpp)
    set(AETHER_OPENGL_AVAILABLE TRUE)
    message(STATUS "Aether Renderer: OpenGL 3.3 + Null")
else()
    message(STATUS "Aether Renderer: Null only")
endif()

if(AETHER_WITH_MINIAUDIO)
    list(APPEND AETHER_ENGINE_SOURCES
        src/audio/miniaudio_impl.cpp
        src/audio/miniaudio_engine.cpp
    )
    message(STATUS "Aether Audio: MiniAudio + Null")
else()
    message(STATUS "Aether Audio: Null")
endif()

if(AETHER_WITH_STB)
    list(APPEND AETHER_ENGINE_SOURCES src/res/stb_impl.cpp)
    message(STATUS "Aether Textures: stb_image")
endif()

set(AETHER_MRUBY_AVAILABLE FALSE)
if(AETHER_WITH_MRUBY)
    aether_require_mruby()
    if(AETHER_MRUBY_AVAILABLE)
        list(APPEND AETHER_ENGINE_SOURCES src/ruby/mruby_vm.cpp)
        message(STATUS "Aether Ruby: mruby + Stub")
    else()
        message(STATUS "Aether Ruby: Stub only (mruby unavailable)")
    endif()
else()
    message(STATUS "Aether Ruby: Stub only")
endif()

# -----------------------------------------------------------------------------
# Library
# -----------------------------------------------------------------------------
add_library(aether_engine STATIC ${AETHER_ENGINE_SOURCES})
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

if(AETHER_WITH_STB AND TARGET aether_stb)
    target_link_libraries(aether_engine PUBLIC aether_stb)
    target_compile_definitions(aether_engine PUBLIC AETHER_WITH_STB=1)
endif()

if(AETHER_WITH_MINIAUDIO AND TARGET aether_miniaudio)
    target_link_libraries(aether_engine PUBLIC aether_miniaudio)
    target_compile_definitions(aether_engine PUBLIC AETHER_WITH_MINIAUDIO=1)
    # miniaudio needs dl/pthread/m on Linux
    if(UNIX AND NOT APPLE)
        target_link_libraries(aether_engine PUBLIC dl m)
    endif()
endif()

if(AETHER_GLFW_AVAILABLE AND TARGET glfw)
    target_link_libraries(aether_engine PUBLIC glfw)
    target_compile_definitions(aether_engine PUBLIC AETHER_WITH_GLFW=1)
    find_package(OpenGL)
    if(OpenGL_FOUND)
        target_link_libraries(aether_engine PUBLIC OpenGL::GL)
    endif()
endif()

if(AETHER_OPENGL_AVAILABLE)
    target_link_libraries(aether_engine PUBLIC aether_glad)
    target_compile_definitions(aether_engine PUBLIC AETHER_WITH_OPENGL=1)
endif()

if(AETHER_MRUBY_AVAILABLE AND TARGET aether_mruby)
    target_link_libraries(aether_engine PUBLIC aether_mruby)
    target_compile_definitions(aether_engine PUBLIC AETHER_WITH_MRUBY=1)
    target_include_directories(aether_engine PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
endif()

aether_apply_common_options(aether_engine)

if(NOT MSVC)
    target_compile_options(aether_engine PRIVATE -Wno-pre-c++17-compat)
    # miniaudio is a large single header with many conversion warnings
    set_source_files_properties(
        src/audio/miniaudio_impl.cpp
        PROPERTIES COMPILE_FLAGS "-Wno-conversion -Wno-sign-conversion -Wno-shadow -Wno-unused-function"
    )
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
