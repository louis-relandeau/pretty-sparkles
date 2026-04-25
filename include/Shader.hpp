#pragma once

#include <glad/glad.h>
#include <string>

class Shader {
public:
  Shader(std::string vertexPath, std::string fragmentPath);
  void use();
  GLint getUniformLocation(const char *name);
  unsigned int ID;
};