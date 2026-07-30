# =============================================================================
# aether_shared – gemeinsame Datentypen Editor ↔ Runtime
# =============================================================================

set(AETHER_SHARED_SOURCES
    src/project_descriptor.cpp
)

add_library(aether_shared STATIC ${AETHER_SHARED_SOURCES})
add_library(aether::shared ALIAS aether_shared)

target_include_directories(aether_shared
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/generated>
)

target_link_libraries(aether_shared
    PUBLIC
        aether_third_party_interface
)

aether_apply_common_options(aether_shared)

set_target_properties(aether_shared PROPERTIES
    OUTPUT_NAME "aether_shared"
    FOLDER "aether"
)
