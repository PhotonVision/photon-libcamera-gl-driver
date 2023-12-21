#pragma once

#include <iostream>
#include <stdexcept>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#define GLERROR() glerror(__LINE__)
#define EGLERROR() eglerror(__LINE__)

inline void glerror(int line) {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::string output;
        output.resize(128);
        snprintf(output.data(), 128, "GL error detected on line %d: 0x%04x\n",
                 line, error);
        std::cout << output << std::endl;
        throw std::runtime_error(output);
    }
}

inline void eglerror(int line) {
    EGLint error = eglGetError();
    if (error != EGL_SUCCESS) {
        std::string output;
        output.resize(128);
        snprintf(output.data(), 128, "EGL error detected on line %d: 0x%04x\n",
                 line, error);
        std::cout << output << std::endl;
        throw std::runtime_error(output);
    }
}