#pragma once

#include <string>
#include <glad/glad.h>

class Shader{
public:
    Shader(std::string vertexPath, std::string fragmentPath);
    void use();
    GLint getUniformLocation(const char* name);
    unsigned int ID;
};