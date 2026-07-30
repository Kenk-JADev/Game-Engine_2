# =============================================================================
# Tests – Catch2 wird bei Bedarf geholt
# =============================================================================

function(aether_add_unit_test name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE aether::engine aether::shared)
    aether_apply_common_options(${name})
    add_test(NAME ${name} COMMAND ${name})
    set_target_properties(${name} PROPERTIES FOLDER "aether/tests")
endfunction()

aether_add_unit_test(aether_smoke_test  unit/smoke_test.cpp)
aether_add_unit_test(aether_core_test   unit/core_test.cpp)
aether_add_unit_test(aether_window_test unit/window_test.cpp)
aether_add_unit_test(aether_render_test unit/render_test.cpp)
aether_add_unit_test(aether_input_test  unit/input_test.cpp)
aether_add_unit_test(aether_audio_test  unit/audio_test.cpp)
aether_add_unit_test(aether_res_test     unit/res_test.cpp)
aether_add_unit_test(aether_ruby_test    unit/ruby_test.cpp)
aether_add_unit_test(aether_project_test unit/project_test.cpp)
aether_add_unit_test(aether_game_test      unit/game_test.cpp)
aether_add_unit_test(aether_phys_nav_test  unit/phys_nav_test.cpp)
aether_add_unit_test(aether_gameplay_test  unit/gameplay_test.cpp)
aether_add_unit_test(aether_save_inv_test    unit/save_inv_test.cpp)
aether_add_unit_test(aether_battle_quest_test unit/battle_quest_test.cpp)
aether_add_unit_test(aether_gltf_test unit/gltf_script_test.cpp)
aether_add_unit_test(aether_pick_test unit/pick_test.cpp)
aether_add_unit_test(aether_event_runner_test unit/event_runner_test.cpp)

add_executable(aether_highlighter_test unit/highlighter_test.cpp
    ${CMAKE_SOURCE_DIR}/editor/src/scripts/script_highlighter.cpp)
target_include_directories(aether_highlighter_test PRIVATE
    ${CMAKE_SOURCE_DIR}/editor/src
    ${CMAKE_SOURCE_DIR}/engine/include)
target_link_libraries(aether_highlighter_test PRIVATE aether::engine aether::shared)
aether_apply_common_options(aether_highlighter_test)
add_test(NAME aether_highlighter_test COMMAND aether_highlighter_test)

# Runtime smoke (headless, few frames) – uses samples if present
add_test(
    NAME aether_runtime_smoke
    COMMAND Game --project ${CMAKE_SOURCE_DIR}/samples/demo_project --headless --max-frames 5
)
set_tests_properties(aether_runtime_smoke PROPERTIES
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

add_test(
    NAME aether_editor_smoke
    COMMAND AetherEditor --headless --new ${CMAKE_BINARY_DIR}/tmp_editor_proj --testplay --max-frames 3
)
set_tests_properties(aether_editor_smoke PROPERTIES
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
