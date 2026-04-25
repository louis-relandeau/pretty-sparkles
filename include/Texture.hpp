#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>

class Texture{
    GLuint tex0;
    int N;
    std::vector<float>& data;
    int unit;
    static int nextUnit;
public:
    Texture(std::vector<float>& data, int N) : N(N), data(data), unit(nextUnit++) {
        glGenTextures(1, &tex0);
        glBindTexture(GL_TEXTURE_2D, tex0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, N, N, 0, GL_RED, GL_FLOAT, data.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    void bind(){
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, tex0);
    }
    void update() {
        glBindTexture(GL_TEXTURE_2D, tex0);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, N, N, GL_RED, GL_FLOAT, data.data());   
    }

    ~Texture(){
        glDeleteTextures(1, &tex0);
    }
};

inline int Texture::nextUnit = 0;