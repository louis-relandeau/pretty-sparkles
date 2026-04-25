// main.cpp — OpenGL matrix heatmap + ImGui overlay
// Dependencies: GLFW, GLAD, ImGui (with backends: imgui_impl_glfw, imgui_impl_opengl3)
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <cmath>
#include <filesystem>
#include <cstdint>

#include "Shader.hpp"
#include "SolverDBM.hpp"
#include "CircularShape.hpp"
#include "Texture.hpp"


std::filesystem::path root = REPO_ROOT;

// Window
int WIN_W = 800;
int WIN_H = 800;

// Called by GLFW whenever the framebuffer is resized
void framebufferSizeCallback(GLFWwindow* /*window*/, int width, int height) {
    WIN_W = width;
    WIN_H = height;
    glViewport(0, 0, width, height);
}


constexpr int N = 300;  // Compile-time constant
std::vector<float> field_arr(N*N, 0);
std::vector<float> geometry(N*N, 0);
std::vector<float> arc(N*N, 0);

// Matrix data
const int ROWS = N;
const int COLS = N;
float matrix[ROWS][COLS];
float field[ROWS][COLS];
float circlular[ROWS][COLS];

// UI-controlled parameters
static bool  animate  = true;
static float time_val = 0.0f;

int main() {
    
    // Build geometry/boundary first, then pass it to the solver
    CircularShape circle(geometry, N);
    std::vector<uint8_t> geometryBoundary(N*N);
    for (int i = 0; i < N*N; ++i) geometryBoundary[i] = geometry[i] ? 1 : 0;

    SolverDBM solver(field_arr, arc, N, geometryBoundary);
    solver.init();

    // solver.print();

    // GLFW
    if (!glfwInit()) { std::cerr << "GLFW init failed\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIN_W, WIN_H, "Matrix Heatmap + ImGui", nullptr, nullptr);
    if (!window) { std::cerr << "Window creation failed\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSwapInterval(1); // vsync

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "GLAD init failed\n"; return -1;
    }
    Shader shader(
        (root / "Shaders/vertexShader.glsl").string(),
        (root / "Shaders/fragmentShader.glsl").string()
    );

    // ImGui setup — must happen after OpenGL context is current
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);  // 'true' installs GLFW callbacks
    ImGui_ImplOpenGL3_Init("#version 330");

    // Quad geometry
    float verts[] = {
        -1.0f, -1.0f,  0.0f, 0.0f, // 2D vertex coordinate, 2D texture coordinate
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
        -1.0f,  1.0f,  0.0f, 1.0f,
    };
    unsigned int indices[] = { 0,1,2, 2,3,0 };

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));
    glEnableVertexAttribArray(1);

    // Texture
    Texture tex0(arc, N);
    Texture tex1(geometry, N);
    Texture tex2(field_arr, N);

    // Shader
    shader.use();
    glUniform1i(shader.getUniformLocation("layer0"), 0);
    glUniform1i(shader.getUniformLocation("layer1"), 1);
    glUniform1i(shader.getUniformLocation("layer2"), 2);
    GLint locColormap = shader.getUniformLocation("uColormap");

    // State driven by ImGui
    int  colormapIdx  = 0;
    // bool smoothFilter = false;
    const char* colormapNames[] = { "Viridis", "Heat", "Grayscale" };

    // Render loop
    while (!glfwWindowShouldClose(window)) {
        solver.step();

        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        // Animate
        if (animate) {
            time_val += 0.02f;
            tex0.update();
            tex1.update();
            tex2.update();
        }

        // --- ImGui frame start ---
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ImGui panel
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(280, 0), ImGuiCond_Once);
        ImGui::Begin("Matrix Controls");

        ImGui::SeparatorText("Colormap");
        ImGui::Combo("##cmap", &colormapIdx, colormapNames, IM_ARRAYSIZE(colormapNames));
        if (ImGui::Button("RESET")) {
            solver.init();
        }
        ImGui::SeparatorText("Filter");
        // if (ImGui::Checkbox("Smooth interpolation", &smoothFilter)) {
        //     GLint f = smoothFilter ? GL_LINEAR : GL_NEAREST;
        //     glBindTexture(GL_TEXTURE_2D, tex0);
        //     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, f);
        //     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, f);
        // }

        ImGui::SeparatorText("Simulation parameters");
        if (!animate) {
            tex0.update();
            tex1.update();
            tex2.update();
        }

        ImGui::SeparatorText("Animation");
        ImGui::Checkbox("Animate", &animate);

        ImGui::SeparatorText("Cell values");
        // if (ImGui::BeginTable("matrix", COLS,
        //     ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY,
        //     ImVec2(0, 160))) {
        //     for (int r = 0; r < ROWS; r++) {
        //         ImGui::TableNextRow();
        //         for (int c = 0; c < COLS; c++) {
        //             ImGui::TableSetColumnIndex(c);
        //             ImGui::Text("%.2f", matrix[r][c]);
        //         }
        //     }
        //     ImGui::EndTable();
        // }

        ImGui::Spacing();
        ImGui::Text("FPS: %.1f", io.Framerate);
        ImGui::End();

        int fbW, fbH;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        glViewport(0, 0, fbW, fbH);
        // --- Render heatmap ---
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        shader.use();
        glUniform1i(locColormap, colormapIdx);
        tex0.bind();
        tex1.bind();
        tex2.bind();

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // --- Render ImGui on top (always last, before swap) ---
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup — ImGui before GLFW
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glfwTerminate();
    return 0;
}
