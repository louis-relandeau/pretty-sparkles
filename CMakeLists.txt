cmake_minimum_required(VERSION 3.10)
project(pretty-sparkles)

set(IMGUI_DIR imgui)
set(SRC_DIR src)

set(SOURCES
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/glad.c
    ${SRC_DIR}/Shader.cpp
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

add_executable(pretty-sparkles ${SOURCES})

# Include directories
target_include_directories(pretty-sparkles PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${CMAKE_SOURCE_DIR}/include
)

# Find GLFW
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)

# Link libraries
target_link_libraries(pretty-sparkles PRIVATE
    ${GLFW_LIBRARIES}
    ${OPENGL_gl_LIBRARY}
    GL
)


target_compile_options(pretty-sparkles PRIVATE -std=c++11 -g -Wall -Wformat)
