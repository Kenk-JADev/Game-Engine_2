add_library(aether_glad STATIC glad/src/glad.c)
target_include_directories(aether_glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/glad/include)
if(NOT MSVC)
    target_compile_options(aether_glad PRIVATE -Wno-pedantic)
endif()
set_target_properties(aether_glad PROPERTIES FOLDER "third_party" POSITION_INDEPENDENT_CODE ON)

add_library(aether_stb INTERFACE)
target_include_directories(aether_stb INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/stb)

add_library(aether_miniaudio INTERFACE)
target_include_directories(aether_miniaudio INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/miniaudio)

add_library(aether_imgui STATIC
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_demo.cpp
)
target_include_directories(aether_imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
if(NOT MSVC)
    target_compile_options(aether_imgui PRIVATE -Wno-conversion -Wno-shadow -Wno-unused-parameter)
endif()
set_target_properties(aether_imgui PROPERTIES FOLDER "third_party" POSITION_INDEPENDENT_CODE ON)

# Function to add imgui glfw/gl3 backends once glfw target exists
function(aether_add_imgui_backends)
    if(TARGET aether_imgui_backends)
        return()
    endif()
    if(NOT TARGET glfw)
        return()
    endif()
    add_library(aether_imgui_backends STATIC
        ${CMAKE_SOURCE_DIR}/third_party/imgui/imgui_impl_glfw.cpp
        ${CMAKE_SOURCE_DIR}/third_party/imgui/imgui_impl_opengl3.cpp
    )
    target_include_directories(aether_imgui_backends PUBLIC
        ${CMAKE_SOURCE_DIR}/third_party/imgui
    )
    target_link_libraries(aether_imgui_backends PUBLIC aether_imgui glfw)
    if(TARGET aether_glad)
        target_link_libraries(aether_imgui_backends PUBLIC aether_glad)
    endif()
    if(NOT MSVC)
        target_compile_options(aether_imgui_backends PRIVATE -Wno-conversion -Wno-shadow -Wno-unused-parameter)
    endif()
endfunction()
