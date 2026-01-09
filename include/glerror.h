/*
 * Copyright (C) Photon Vision.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#define GLERROR() glerror(__LINE__)
#define EGLERROR() eglerror(__LINE__)

inline void glerror(int line) {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::string output;
        output.resize(128);
        std::snprintf(output.data(), output.size(),
                      "GL error detected on line %d: 0x%04x\n", line, error);
        std::cout << output << std::endl;
        throw std::runtime_error(output);
    }
}

inline void eglerror(int line) {
    EGLint error = eglGetError();
    if (error != EGL_SUCCESS) {
        std::string output;
        output.resize(128);
        std::snprintf(output.data(), output.size(),
                      "EGL error detected on line %d: 0x%04x\n", line, error);
        std::cout << output << std::endl;
        throw std::runtime_error(output);
    }
}
