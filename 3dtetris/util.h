#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <random>
#include <iostream>

int gKeyPressed[512];

void checkGLError()
{
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        std::cerr << "OpenGL Error: " << error << std::endl;
    }
}

int getRandomInt(int from, int to) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(from, to);
    return dis(gen);
}