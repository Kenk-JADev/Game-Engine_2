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

# Threads (Standard)
find_package(Threads REQUIRED)
