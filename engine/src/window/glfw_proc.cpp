/**
 * @file glfw_proc.cpp
 * @brief Exponiert glfwGetProcAddress für GLAD.
 */
#if defined(AETHER_WITH_GLFW)
#  include <GLFW/glfw3.h>

extern "C" void* aether_glfw_get_proc_address(const char* name) {
    return reinterpret_cast<void*>(glfwGetProcAddress(name));
}
#endif
