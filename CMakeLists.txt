cmake_minimum_required(VERSION 3.10)
project(pretty-sparkles)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")

# ========= Main =========

set(IMGUI_DIR imgui)
set(SRC_DIR src)
add_subdirectory(glfw)

set(SOURCES
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/glad.c
    ${SRC_DIR}/Shader.cpp
    ${SRC_DIR}/SolverDBM.cpp
    ${SRC_DIR}/CircularShape.cpp
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

add_executable(pretty-sparkles ${SOURCES})

find_package(OpenMP REQUIRED)

# Include directories
target_include_directories(pretty-sparkles PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${CMAKE_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(pretty-sparkles PRIVATE
    glfw
    GL
    OpenMP::OpenMP_CXX
)

target_compile_options(pretty-sparkles PRIVATE -g -Wall -Wformat)

target_compile_definitions(pretty-sparkles PRIVATE
    REPO_ROOT=\"${CMAKE_SOURCE_DIR}\"
)

# ========= Benchmark =========

add_executable(benchmark-test
    ${SRC_DIR}/benchmark.cpp
    ${SRC_DIR}/SolverDBM.cpp
)

target_include_directories(benchmark-test PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_options(benchmark-test PRIVATE -g -Wall -Wformat)
