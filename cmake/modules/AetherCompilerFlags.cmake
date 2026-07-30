# =============================================================================
# AetherCompilerFlags.cmake
# Gemeinsame Warn- und Optimierungsflags für alle Aether-Targets.
# =============================================================================

function(aether_apply_common_options target_name)
    if(NOT TARGET ${target_name})
        message(FATAL_ERROR "aether_apply_common_options: target '${target_name}' does not exist")
    endif()

    target_compile_features(${target_name} PUBLIC cxx_std_20)

    # Debug/Release-Makros
    target_compile_definitions(${target_name}
        PUBLIC
            $<$<CONFIG:Debug>:AETHER_DEBUG=1>
            $<$<NOT:$<CONFIG:Debug>>:AETHER_DEBUG=0>
            $<$<CONFIG:Release>:AETHER_RELEASE=1>
    )

    if(MSVC)
        target_compile_options(${target_name} PRIVATE
            /W4
            /permissive-
            /Zc:__cplusplus
            /utf-8
            /MP
            $<$<CONFIG:Debug>:/Od /Zi>
            $<$<CONFIG:Release>:/O2 /Ob2>
            $<$<CONFIG:RelWithDebInfo>:/O2 /Zi>
        )
        target_compile_definitions(${target_name} PRIVATE
            _CRT_SECURE_NO_WARNINGS
            NOMINMAX
            WIN32_LEAN_AND_MEAN
        )
        if(AETHER_WARNINGS_AS_ERRORS)
            target_compile_options(${target_name} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target_name} PRIVATE
            -Wall -Wextra -Wpedantic
            -Wconversion -Wshadow
            -Wno-unused-parameter
            $<$<CONFIG:Debug>:-O0 -g>
            $<$<CONFIG:Release>:-O2>
            $<$<CONFIG:RelWithDebInfo>:-O2 -g>
        )
        if(AETHER_WARNINGS_AS_ERRORS)
            target_compile_options(${target_name} PRIVATE -Werror)
        endif()
    endif()

    if(AETHER_ENABLE_ASAN AND NOT MSVC)
        target_compile_options(${target_name} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(${target_name} PRIVATE -fsanitize=address)
    endif()

    # Position-independent code for static libs used in shared contexts
    set_target_properties(${target_name} PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )
endfunction()
