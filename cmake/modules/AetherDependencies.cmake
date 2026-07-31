# =============================================================================
# AetherDependencies.cmake
# Zentrale Verwaltung externer Abhängigkeiten.
#
# Phase 1 (Module 3–5): Minimale / optionale Deps.
# Später: GLFW, GLAD, glm, nlohmann_json, stb, assimp, miniaudio, mruby …
# via FetchContent oder find_package.
# =============================================================================

include(FetchContent)

# Einheitliche FetchContent-Einstellungen
set(FETCHCONTENT_QUIET OFF)

# -----------------------------------------------------------------------------
# nlohmann_json – früh benötigt (Projektformat, Config)
# -----------------------------------------------------------------------------
find_package(nlohmann_json 3.11 QUIET)
if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann_json not found – fetching via FetchContent")
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG        v3.11.3
        GIT_SHALLOW    TRUE
    )
    # Nur Library, keine Tests/Install-Noise
    set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
    set(JSON_Install    OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# -----------------------------------------------------------------------------
# Platzhalter-Interface für künftige Deps
# Targets können aether_third_party_interface linken.
# -----------------------------------------------------------------------------
if(NOT TARGET aether_third_party_interface)
    add_library(aether_third_party_interface INTERFACE)
    # nlohmann_json::nlohmann_json ist header-only INTERFACE
    if(TARGET nlohmann_json::nlohmann_json)
        target_link_libraries(aether_third_party_interface INTERFACE nlohmann_json::nlohmann_json)
    endif()
endif()

# Hilfsfunktion: Dependency erst anfordern, wenn Subsystem gebaut wird
function(aether_require_glfw)
    if(TARGET glfw)
        set(AETHER_GLFW_AVAILABLE TRUE PARENT_SCOPE)
        return()
    endif()
    find_package(glfw3 3.3 QUIET)
    if(TARGET glfw)
        set(AETHER_GLFW_AVAILABLE TRUE PARENT_SCOPE)
        return()
    endif()
    # Ohne X11/Wayland-Headers macht FetchContent keinen Sinn – vorher prüfen
    find_path(AETHER_X11_INCLUDE X11/Xlib.h)
    find_library(AETHER_X11_LIB X11)
    if(NOT AETHER_X11_INCLUDE OR NOT AETHER_X11_LIB)
        message(WARNING
            "GLFW requested but X11 dev packages not found. "
            "Building without GLFW (NullWindow only). "
            "Install libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev")
        set(AETHER_GLFW_AVAILABLE FALSE PARENT_SCOPE)
        return()
    endif()

    message(STATUS "GLFW not found – fetching via FetchContent")
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.4
        GIT_SHALLOW    TRUE
    )
    set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_INSTALL  OFF CACHE BOOL "" FORCE)
    # Wayland deaktivieren: Aether zielt auf X11/Cocoa. Sonst schlaegt der
    # GLFW-Configure fehl, wenn Wayland-Header vorhanden sind, aber
    # wayland-scanner fehlt (klassischer CI-Fehler auf ubuntu-Images).
    set(GLFW_BUILD_WAYLAND  OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glfw)
    if(TARGET glfw)
        set(AETHER_GLFW_AVAILABLE TRUE PARENT_SCOPE)
    else()
        set(AETHER_GLFW_AVAILABLE FALSE PARENT_SCOPE)
    endif()
endfunction()

function(aether_require_glm)
    if(TARGET glm::glm)
        set(AETHER_GLM_AVAILABLE TRUE PARENT_SCOPE)
        return()
    endif()
    find_package(glm QUIET)
    if(TARGET glm::glm)
        set(AETHER_GLM_AVAILABLE TRUE PARENT_SCOPE)
        return()
    endif()
    message(STATUS "glm not found – fetching via FetchContent")
    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG        1.0.1
        GIT_SHALLOW    TRUE
    )
    set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLM_BUILD_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glm)
    if(TARGET glm::glm)
        set(AETHER_GLM_AVAILABLE TRUE PARENT_SCOPE)
    elseif(TARGET glm)
        if(NOT TARGET glm::glm)
            add_library(glm::glm ALIAS glm)
        endif()
        set(AETHER_GLM_AVAILABLE TRUE PARENT_SCOPE)
    else()
        # Fallback: header-only interface from source dir
        if(DEFINED glm_SOURCE_DIR)
            add_library(aether_glm INTERFACE)
            target_include_directories(aether_glm INTERFACE ${glm_SOURCE_DIR})
            add_library(glm::glm ALIAS aether_glm)
            set(AETHER_GLM_AVAILABLE TRUE PARENT_SCOPE)
        else()
            set(AETHER_GLM_AVAILABLE FALSE PARENT_SCOPE)
        endif()
    endif()
endfunction()

# -----------------------------------------------------------------------------
# mruby – optional, requires host Ruby (rake) to compile the VM
# -----------------------------------------------------------------------------
function(aether_require_mruby)
    if(TARGET aether_mruby)
        set(AETHER_MRUBY_AVAILABLE TRUE PARENT_SCOPE)
        return()
    endif()

    find_program(AETHER_RUBY_EXECUTABLE NAMES ruby)
    find_program(AETHER_RAKE_EXECUTABLE NAMES rake)
    if(NOT AETHER_RUBY_EXECUTABLE)
        message(STATUS "mruby: host ruby not found – StubRubyVM only")
        set(AETHER_MRUBY_AVAILABLE FALSE PARENT_SCOPE)
        return()
    endif()

    # Prefer vendored tree, else FetchContent
    set(_mruby_src "")
    if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/mruby-src/Rakefile")
        set(_mruby_src "${CMAKE_SOURCE_DIR}/third_party/mruby-src")
        message(STATUS "mruby: using vendored third_party/mruby-src")
    else()
        message(STATUS "mruby: fetching via FetchContent")
        FetchContent_Declare(
            mruby_fc
            GIT_REPOSITORY https://github.com/mruby/mruby.git
            GIT_TAG        3.3.0
            GIT_SHALLOW    TRUE
        )
        FetchContent_GetProperties(mruby_fc)
        if(NOT mruby_fc_POPULATED)
            FetchContent_Populate(mruby_fc)
        endif()
        set(_mruby_src "${mruby_fc_SOURCE_DIR}")
    endif()

    set(_mruby_build "${CMAKE_BINARY_DIR}/mruby-build")
    set(_mruby_lib   "${_mruby_build}/host/lib/libmruby.a")
    if(MSVC)
        set(_mruby_lib "${_mruby_build}/host/lib/libmruby.lib")
    endif()
    set(_mruby_cfg   "${CMAKE_SOURCE_DIR}/cmake/mruby_build_config.rb")
    set(_mruby_script "${CMAKE_BINARY_DIR}/build_mruby.cmake")

    # Generate a small cmake driver so paths with spaces work reliably
    file(WRITE "${_mruby_script}" "
set(ENV{MRUBY_CONFIG} \"${_mruby_cfg}\")
set(ENV{MRUBY_BUILD_DIR} \"${_mruby_build}\")
if(EXISTS \"${AETHER_RAKE_EXECUTABLE}\")
  execute_process(COMMAND \"${AETHER_RAKE_EXECUTABLE}\"
    WORKING_DIRECTORY \"${_mruby_src}\"
    RESULT_VARIABLE rv)
else()
  execute_process(COMMAND \"${AETHER_RUBY_EXECUTABLE}\" \"./minirake\"
    WORKING_DIRECTORY \"${_mruby_src}\"
    RESULT_VARIABLE rv)
endif()
if(NOT rv EQUAL 0)
  message(FATAL_ERROR \"mruby rake failed: \${rv}\")
endif()
")

    include(ExternalProject)
    ExternalProject_Add(mruby_ext
        SOURCE_DIR ${_mruby_src}
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ${CMAKE_COMMAND} -P ${_mruby_script}
        BUILD_IN_SOURCE 0
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS ${_mruby_lib}
        LOG_BUILD 1
    )

    add_library(aether_mruby STATIC IMPORTED GLOBAL)
    set_target_properties(aether_mruby PROPERTIES
        IMPORTED_LOCATION ${_mruby_lib}
        INTERFACE_INCLUDE_DIRECTORIES "${_mruby_src}/include"
    )
    add_dependencies(aether_mruby mruby_ext)

    # mruby may need libm / dl
    if(UNIX AND NOT APPLE)
        set_property(TARGET aether_mruby APPEND PROPERTY INTERFACE_LINK_LIBRARIES m dl)
    endif()

    set(AETHER_MRUBY_AVAILABLE TRUE PARENT_SCOPE)
    set(AETHER_MRUBY_INCLUDE_DIR "${_mruby_src}/include" PARENT_SCOPE)
    message(STATUS "mruby: will build into ${_mruby_build}")
endfunction()

# Threads (Standard)
find_package(Threads REQUIRED)
