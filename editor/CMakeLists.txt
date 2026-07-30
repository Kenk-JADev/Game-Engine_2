# =============================================================================
# AetherEditor – Visueller RPG-Maker-Editor
# =============================================================================

set(AETHER_EDITOR_SOURCES
    src/app/main.cpp
    src/ui/editor_app.cpp
    src/ui/undo_stack.cpp
    src/scripts/script_highlighter.cpp
    src/scripts/script_debugger.cpp
    src/export/exporter.cpp
)

add_executable(AetherEditor ${AETHER_EDITOR_SOURCES})

target_include_directories(AetherEditor
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(AetherEditor
    PRIVATE
        aether::engine
        aether::shared
)

# ImGui UI when available
if(TARGET aether_imgui)
    target_link_libraries(AetherEditor PRIVATE aether_imgui)
    target_compile_definitions(AetherEditor PRIVATE AETHER_WITH_IMGUI=1)
    if(COMMAND aether_add_imgui_backends)
        aether_add_imgui_backends()
    endif()
    if(TARGET aether_imgui_backends)
        target_link_libraries(AetherEditor PRIVATE aether_imgui_backends)
        if(TARGET aether_glad)
            target_link_libraries(AetherEditor PRIVATE aether_glad)
        endif()
    endif()
endif()

aether_apply_common_options(AetherEditor)

set_target_properties(AetherEditor PROPERTIES
    OUTPUT_NAME "AetherEditor"
    FOLDER "aether"
    WIN32_EXECUTABLE FALSE
)

if(EXISTS "${CMAKE_SOURCE_DIR}/assets/engine")
    aether_copy_dir_postbuild(AetherEditor
        "${CMAKE_SOURCE_DIR}/assets/engine"
        "assets/engine")
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/assets/editor")
    aether_copy_dir_postbuild(AetherEditor
        "${CMAKE_SOURCE_DIR}/assets/editor"
        "assets/editor")
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/templates")
    aether_copy_dir_postbuild(AetherEditor
        "${CMAKE_SOURCE_DIR}/templates"
        "templates")
endif()

install(TARGETS AetherEditor RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
