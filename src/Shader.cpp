#include "Shader.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <glad/glad.h>

// read shader from file
std::string loadShader(const std::string& path)
{
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Failed to open shader file: " << path << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    int ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512]; glGetShaderInfoLog(s, 512, nullptr, log);
        std::cerr << "Shader error: " << log << "\n";
    }
    return s;
}

Shader::Shader(std::string vertexPath, std::string fragmentPath){

    // Load the vertex and fragment shaders from file
    std::string vertexCode = loadShader(vertexPath);
    std::string fragmentCode = loadShader(fragmentPath);

    if (vertexCode.empty()) {
        std::cerr << "Vertex shader source is empty! Check file: " << vertexPath << std::endl;
    }
    if (fragmentCode.empty()) {
        std::cerr << "Fragment shader source is empty! Check file: " << fragmentPath << std::endl;
    }

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    GLuint vShader = compileShader(GL_VERTEX_SHADER, vShaderCode);
    std::cout << fShaderCode << std::endl;

    GLuint fShader = compileShader(GL_FRAGMENT_SHADER, fShaderCode);

    ID = glCreateProgram();
    glAttachShader(ID, vShader);
    glAttachShader(ID, fShader);
    glLinkProgram(ID);
    
    GLint success;
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if(!success) {
        char infoLog[512];
        glGetProgramInfoLog(ID, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

    }

    glDeleteShader(vShader);
    glDeleteShader(fShader);
    std::cout << "Finished compiling shaders" << std::endl;
}

void Shader::use() {
    glUseProgram(ID);
}

GLint Shader::getUniformLocation(const char* name){

    return glGetUniformLocation(ID, name);   
}