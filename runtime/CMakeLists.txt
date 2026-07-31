# =============================================================================
# Game – Schlanke Runtime (Game.exe)
# =============================================================================

set(AETHER_RUNTIME_SOURCES
    src/main.cpp
    src/bootstrap.cpp
)

add_executable(Game ${AETHER_RUNTIME_SOURCES})

target_include_directories(Game
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(Game
    PRIVATE
        aether::engine
        aether::shared
)

aether_apply_common_options(Game)

set_target_properties(Game PROPERTIES
    OUTPUT_NAME "Game"
    FOLDER "aether"
)

if(EXISTS "${CMAKE_SOURCE_DIR}/assets/engine")
    aether_copy_dir_postbuild(Game
        "${CMAKE_SOURCE_DIR}/assets/engine"
        "assets/engine")
endif()

# Demo-Projekt neben die Runtime kopieren (fuer Doppelklick-Start / Testspiel)
if(EXISTS "${CMAKE_SOURCE_DIR}/samples/demo_project")
    aether_copy_dir_postbuild(Game
        "${CMAKE_SOURCE_DIR}/samples/demo_project"
        "samples/demo_project")
endif()

# Doppelklick-Startskripte (Windows) neben die Runtime kopieren
if(WIN32)
    aether_copy_dir_postbuild(Game
        "${CMAKE_SOURCE_DIR}/tools/scripts"
        ".")
endif()

install(TARGETS Game RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
