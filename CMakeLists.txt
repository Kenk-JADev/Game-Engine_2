# =============================================================================
# AetherRPG Maker – Root CMakeLists
# =============================================================================
cmake_minimum_required(VERSION 3.21)

project(AetherRPG
    VERSION 0.1.0
    DESCRIPTION "AetherRPG Maker – eigenständige 3D-RPG-Maker-Engine"
    LANGUAGES CXX C
)

# -----------------------------------------------------------------------------
# Globale Optionen
# -----------------------------------------------------------------------------
option(AETHER_BUILD_EDITOR   "Build Aether Editor"              ON)
option(AETHER_BUILD_RUNTIME  "Build Game runtime"               ON)
option(AETHER_BUILD_TESTS    "Build unit/integration tests"     ON)
option(AETHER_BUILD_SAMPLES  "Copy/install sample projects"     ON)
option(AETHER_WARNINGS_AS_ERRORS "Treat warnings as errors"     OFF)
option(AETHER_ENABLE_ASAN    "Enable AddressSanitizer (Debug)"  OFF)

# Nur 64-Bit
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "AetherRPG requires a 64-bit toolchain.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Ausgabeordner vereinheitlichen
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Multi-config (MSVC) – pro Config Unterordner vermeiden für einfacheres Dev
foreach(cfg DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    string(TOUPPER ${cfg} cfg_u)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${cfg_u} ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${cfg_u} ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${cfg_u} ${CMAKE_BINARY_DIR}/lib)
endforeach()

# -----------------------------------------------------------------------------
# Eigene CMake-Module
# -----------------------------------------------------------------------------
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

include(AetherCompilerFlags)
include(AetherDependencies)

# -----------------------------------------------------------------------------
# Version-Header generieren
# -----------------------------------------------------------------------------
configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/aether_version.hpp.in
    ${CMAKE_BINARY_DIR}/generated/aether/version.hpp
    @ONLY
)

# -----------------------------------------------------------------------------
# Assets in Build-Tree spiegeln (Development)
# -----------------------------------------------------------------------------
function(aether_copy_dir_postbuild target src dst_rel)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${target}>/${dst_rel}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${src}"
            "$<TARGET_FILE_DIR:${target}>/${dst_rel}"
        COMMENT "Copy ${dst_rel} for ${target}"
        VERBATIM
    )
endfunction()

# -----------------------------------------------------------------------------
# Targets
# -----------------------------------------------------------------------------
add_subdirectory(third_party)
add_subdirectory(shared)
add_subdirectory(engine)

if(AETHER_BUILD_EDITOR)
    add_subdirectory(editor)
endif()

if(AETHER_BUILD_RUNTIME)
    add_subdirectory(runtime)
endif()

if(AETHER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# -----------------------------------------------------------------------------
# Install-Regeln (Export-Grundlage)
# -----------------------------------------------------------------------------
include(GNUInstallDirs)

install(DIRECTORY assets/engine DESTINATION ${CMAKE_INSTALL_DATADIR}/aether)
install(DIRECTORY templates DESTINATION ${CMAKE_INSTALL_DATADIR}/aether)
install(DIRECTORY plugins DESTINATION ${CMAKE_INSTALL_DATADIR}/aether)

if(AETHER_BUILD_SAMPLES)
    install(DIRECTORY samples DESTINATION ${CMAKE_INSTALL_DATADIR}/aether)
endif()

# -----------------------------------------------------------------------------
# Zusammenfassung
# -----------------------------------------------------------------------------
message(STATUS "========================================================")
message(STATUS " AetherRPG ${PROJECT_VERSION}")
message(STATUS "--------------------------------------------------------")
message(STATUS " Compiler        : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS " Build type      : ${CMAKE_BUILD_TYPE}")
message(STATUS " Editor          : ${AETHER_BUILD_EDITOR}")
message(STATUS " Runtime         : ${AETHER_BUILD_RUNTIME}")
message(STATUS " Tests           : ${AETHER_BUILD_TESTS}")
message(STATUS " Warnings=errors : ${AETHER_WARNINGS_AS_ERRORS}")
message(STATUS "========================================================")
