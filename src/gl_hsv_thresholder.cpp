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

#include "gl_hsv_thresholder.h"

#include <libdrm/drm_fourcc.h>

#include <iostream>
#include <stdexcept>

#include <EGL/eglext.h>
#include <GLES2/gl2ext.h>

#include "gl_shader_source.h"
#include "glerror.h"

#define GLERROR() glerror(__LINE__)
#define EGLERROR() eglerror(__LINE__)

GLuint make_shader(GLenum type, const char *source) {
    auto shader = glCreateShader(type);

    // void *ctx = eglGetCurrentContext();
    // std::printf("Shader idx: %i context ptr: %lu\n", (int) shader,
    // (size_t)ctx);

    if (!shader) {
        throw std::runtime_error("failed to create shader");
    }
    glShaderSource(shader, 1, &source, nullptr);
    GLERROR();
    glCompileShader(shader);
    GLERROR();

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        GLint log_size;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_size);

        std::string out;
        out.resize(log_size);
        glGetShaderInfoLog(shader, log_size, nullptr, out.data());

        glDeleteShader(shader);
        throw std::runtime_error("failed to compile shader with error: " + out);
    }

    return shader;
}

GLuint make_program(const char *vertex_source, const char *fragment_source) {
    auto vertex_shader = make_shader(GL_VERTEX_SHADER, vertex_source);
    auto fragment_shader = make_shader(GL_FRAGMENT_SHADER, fragment_source);

    auto program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    GLERROR();
    glAttachShader(program, fragment_shader);
    GLERROR();
    glLinkProgram(program);
    GLERROR();

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (!status) {
        GLint log_size;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_size);

        std::string out;
        out.resize(log_size);
        glGetProgramInfoLog(program, log_size, nullptr, out.data());

        throw std::runtime_error("failed to link program with error: " + out);
    }
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return program;
}

GlHsvThresholder::GlHsvThresholder(int width, int height)
    : m_width(width), m_height(height) {
    m_status = createHeadless();
    m_context = m_status.context;
    m_display = m_status.display;
}

GlHsvThresholder::~GlHsvThresholder() {
    for (auto &program : m_programs)
        glDeleteProgram(program);

    glDeleteBuffers(1, &m_quad_vbo);
    for (const auto &[key, value] : m_framebuffers) {
        glDeleteFramebuffers(1, &value);
    }
    destroyHeadless(m_status);
}

// static void on_gl_error(EGLenum error,const char *command,EGLint
// messageType,EGLLabelKHR threadLabel,EGLLabelKHR objectLabel,const char*
// message)
// {
//     (void) error;
//     (void) command;
//     (void) messageType;
//     (void) threadLabel;
//     (void) objectLabel;
//     std::printf("Error111: %s\n", message);
// }

void GlHsvThresholder::start(const std::vector<int> &output_buf_fds) {
    static auto glEGLImageTargetTexture2DOES =
        (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress(
            "glEGLImageTargetTexture2DOES");
    static auto eglCreateImageKHR =
        (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
    static auto eglDestroyImageKHR =
        (PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");

    if (!eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, m_context)) {
        throw std::runtime_error("failed to bind egl context");
    }
    EGLERROR();

    // static auto glDebugMessageCallbackKHR =
    //         (PFNEGLDEBUGMESSAGECONTROLKHRPROC)eglGetProcAddress("glDebugMessageCallbackKHR");
    // glEnable(GL_DEBUG_OUTPUT_KHR);
    // GLERROR();
    // glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_KHR);
    // GLERROR();
    // glDebugMessageCallbackKHR(on_gl_error, nullptr);
    // GLERROR();

    m_programs.reserve(5);
    m_programs[0] = make_program(VERTEX_SOURCE, NONE_FRAGMENT_SOURCE);
    m_programs[1] = make_program(VERTEX_SOURCE, HSV_FRAGMENT_SOURCE);
    m_programs[2] = make_program(VERTEX_SOURCE, GRAY_FRAGMENT_SOURCE);
    m_programs[3] = make_program(VERTEX_SOURCE, TILING_FRAGMENT_SOURCE);
    m_programs[4] = make_program(VERTEX_SOURCE, THRESHOLDING_FRAGMENT_SOURCE);

    for (auto fd : output_buf_fds) {
        GLuint out_tex;
        glGenTextures(1, &out_tex);
        GLERROR();
        glBindTexture(GL_TEXTURE_2D, out_tex);
        GLERROR();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        GLERROR();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        GLERROR();

        const EGLint image_attribs[] = {EGL_WIDTH,
                                        static_cast<EGLint>(m_width),
                                        EGL_HEIGHT,
                                        static_cast<EGLint>(m_height),
                                        EGL_LINUX_DRM_FOURCC_EXT,
                                        DRM_FORMAT_ABGR8888,
                                        EGL_DMA_BUF_PLANE0_FD_EXT,
                                        static_cast<EGLint>(fd),
                                        EGL_DMA_BUF_PLANE0_OFFSET_EXT,
                                        0,
                                        EGL_DMA_BUF_PLANE0_PITCH_EXT,
                                        static_cast<EGLint>(m_width * 4),
                                        EGL_NONE};
        auto image =
            eglCreateImageKHR(m_display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT,
                              nullptr, image_attribs);
        EGLERROR();
        if (!image) {
            throw std::runtime_error("failed to import fd " +
                                     std::to_string(fd));
        }

        glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image);
        GLERROR();

        eglDestroyImageKHR(m_display, image);
        GLERROR();

        GLuint framebuffer;
        glGenFramebuffers(1, &framebuffer);
        GLERROR();
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        GLERROR();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, out_tex, 0);
        GLERROR();

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
            GL_FRAMEBUFFER_COMPLETE) {
            throw std::runtime_error("failed to complete framebuffer");
        }

        m_framebuffers.emplace(fd, framebuffer);
        m_renderable.push(fd);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    {
        static GLfloat quad_varray[] = {
            -1.0f, -1.0f, 1.0f, 1.0f, 1.0f,  -1.0f,
            -1.0f, 1.0f,  1.0f, 1.0f, -1.0f, -1.0f,
        };

        GLuint quad_vbo;
        glGenBuffers(1, &quad_vbo);
        GLERROR();
        glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
        GLERROR();
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad_varray), quad_varray,
                     GL_STATIC_DRAW);
        GLERROR();

        m_quad_vbo = quad_vbo;
    }

    {
        GLuint grayscale_texture;
        glGenTextures(1, &grayscale_texture);
        GLERROR();
        glBindTexture(GL_TEXTURE_2D, grayscale_texture);
        GLERROR();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        GLERROR();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        GLERROR();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, nullptr);
        GLERROR();

        m_grayscale_texture = grayscale_texture;
    }

    {
        GLuint grayscale_buffer;
        glGenFramebuffers(1, &grayscale_buffer);
        GLERROR();
        glBindFramebuffer(GL_FRAMEBUFFER, grayscale_buffer);
        GLERROR();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, m_grayscale_texture, 0);
        GLERROR();

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
            GL_FRAMEBUFFER_COMPLETE) {
            throw std::runtime_error("failed to complete grayscale_buffer");
        }

        m_grayscale_buffer = grayscale_buffer;
    }

    {
        GLuint min_max_texture;
        glGenTextures(1, &min_max_texture);
        GLERROR();
        glBindTexture(GL_TEXTURE_2D, min_max_texture);
        GLERROR();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        GLERROR();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        GLERROR();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        GLERROR();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        GLERROR();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_width / 4, m_height / 4, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        GLERROR();

        m_min_max_texture = min_max_texture;
    }

    {
        GLuint min_max_framebuffer;
        glGenFramebuffers(1, &min_max_framebuffer);
        GLERROR();
        glBindFramebuffer(GL_FRAMEBUFFER, min_max_framebuffer);
        GLERROR();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, m_min_max_texture, 0);
        GLERROR();

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
            GL_FRAMEBUFFER_COMPLETE) {
            throw std::runtime_error("failed to complete grayscale_buffer");
        }

        m_min_max_framebuffer = min_max_framebuffer;
    }
}

void GlHsvThresholder::release() {
    if (!eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                        EGL_NO_CONTEXT)) {
        throw std::runtime_error("failed to bind egl context");
    }
}

int GlHsvThresholder::testFrame(
    const std::array<GlHsvThresholder::DmaBufPlaneData, 3> &yuv_plane_data,
    EGLint encoding, EGLint range, ProcessType type) {
    static auto glEGLImageTargetTexture2DOES =
        (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress(
            "glEGLImageTargetTexture2DOES");
    static auto eglCreateImageKHR =
        (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
    static auto eglDestroyImageKHR =
        (PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");

    if (!glEGLImageTargetTexture2DOES) {
        throw std::runtime_error(
            "cannot get address of glEGLImageTargetTexture2DOES");
    }

    int framebuffer_fd;
    {
        std::scoped_lock lock(m_renderable_mutex);
        if (!m_renderable.empty()) {
            framebuffer_fd = m_renderable.front();
            // std::cout << "yes framebuffer" << std::endl;
            m_renderable.pop();
        } else {
            // std::cout << "no framebuffer, skipping" << std::endl;
            return 0;
        }
    }

    // Begin code setup that does not change with type

    EGLint attribs[] = {EGL_WIDTH,
                        m_width,
                        EGL_HEIGHT,
                        m_height,
                        EGL_LINUX_DRM_FOURCC_EXT,
                        DRM_FORMAT_YUV420,
                        EGL_DMA_BUF_PLANE0_FD_EXT,
                        yuv_plane_data[0].fd,
                        EGL_DMA_BUF_PLANE0_OFFSET_EXT,
                        yuv_plane_data[0].offset,
                        EGL_DMA_BUF_PLANE0_PITCH_EXT,
                        yuv_plane_data[0].pitch,
                        EGL_DMA_BUF_PLANE1_FD_EXT,
                        yuv_plane_data[1].fd,
                        EGL_DMA_BUF_PLANE1_OFFSET_EXT,
                        yuv_plane_data[1].offset,
                        EGL_DMA_BUF_PLANE1_PITCH_EXT,
                        yuv_plane_data[1].pitch,
                        EGL_DMA_BUF_PLANE2_FD_EXT,
                        yuv_plane_data[2].fd,
                        EGL_DMA_BUF_PLANE2_OFFSET_EXT,
                        yuv_plane_data[2].offset,
                        EGL_DMA_BUF_PLANE2_PITCH_EXT,
                        yuv_plane_data[2].pitch,
                        EGL_YUV_COLOR_SPACE_HINT_EXT,
                        encoding,
                        EGL_SAMPLE_RANGE_HINT_EXT,
                        range,
                        EGL_NONE};

    auto image = eglCreateImageKHR(m_display, EGL_NO_CONTEXT,
                                   EGL_LINUX_DMA_BUF_EXT, nullptr, attribs);
    EGLERROR();
    if (!image) {
        throw std::runtime_error("failed to import fd " +
                                 std::to_string(yuv_plane_data[0].fd));
    }

    GLuint texture;
    glGenTextures(1, &texture);
    GLERROR();
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture);
    GLERROR();
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    GLERROR();
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    GLERROR();
    glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, image);
    GLERROR();
    eglDestroyImageKHR(m_display, image);
    EGLERROR();
    // End portion of code that does not change with type

    GLuint initial_program = -1;

    if (type == ProcessType::None) {
        initial_program = m_programs[0];
    } else if (type == ProcessType::Hsv) {
        initial_program = m_programs[1];
    } else if (type == ProcessType::Gray || type == ProcessType::Adaptive) {
        initial_program = m_programs[2];
    }

    glUseProgram(initial_program);
    GLERROR();
    glUniform1i(glGetUniformLocation(initial_program, "tex"), 0);
    GLERROR();

    if (type == ProcessType::Hsv) {
        auto lll = glGetUniformLocation(initial_program, "lowerThresh");
        auto uuu = glGetUniformLocation(initial_program, "upperThresh");
        auto invert = glGetUniformLocation(initial_program, "invertHue");

        std::lock_guard lock{m_hsv_mutex};
        glUniform3f(lll, m_hsvLower[0], m_hsvLower[1], m_hsvLower[2]);
        GLERROR();
        glUniform3f(uuu, m_hsvUpper[0], m_hsvUpper[1], m_hsvUpper[2]);
        GLERROR();
        glUniform1i(invert, m_invertHue);
        GLERROR();
    }

    glActiveTexture(GL_TEXTURE0);
    GLERROR();
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture);
    GLERROR();

    glBindBuffer(GL_ARRAY_BUFFER, m_quad_vbo);
    GLERROR();

    auto attr_loc = glGetAttribLocation(initial_program, "vertex");
    glEnableVertexAttribArray(attr_loc);
    GLERROR();
    glVertexAttribPointer(attr_loc, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    GLERROR();

    if (type != ProcessType::Adaptive) {
        auto out_framebuffer = m_framebuffers.at(framebuffer_fd);
        glBindFramebuffer(GL_FRAMEBUFFER, out_framebuffer);
        GLERROR();

        glViewport(0, 0, m_width, m_height);
        GLERROR();
        glClear(GL_COLOR_BUFFER_BIT);
        GLERROR();

        glDrawArrays(GL_TRIANGLES, 0, 6);
        GLERROR();

        glFinish();
        GLERROR();

        glDeleteTextures(1, &texture);
        GLERROR();

        return framebuffer_fd;
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, m_grayscale_buffer);
        GLERROR();

        glViewport(0, 0, m_width, m_height);
        GLERROR();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        GLERROR();

        glDrawArrays(GL_TRIANGLES, 0, 6);
        GLERROR();

        glBindFramebuffer(GL_FRAMEBUFFER, m_min_max_framebuffer);
        GLERROR();

        glViewport(0, 0, m_width / 4, m_height / 4);
        GLERROR();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        GLERROR();

        glActiveTexture(GL_TEXTURE0);
        GLERROR();
        glBindTexture(GL_TEXTURE_2D, m_grayscale_texture);
        GLERROR();

        glUseProgram(m_programs[3]);
        GLERROR();

        glUniform1i(glGetUniformLocation(m_programs[3], "tex"), 0);
        GLERROR();

        auto res = glGetUniformLocation(m_programs[3], "resolution_in");
        GLERROR();
        glUniform2f(res, m_width, m_height);
        GLERROR();

        glDrawArrays(GL_TRIANGLES, 0, 6);
        GLERROR();

        // TODO: This finish may be unneeded
        glFinish();
        GLERROR();

        auto out_framebuffer = m_framebuffers.at(framebuffer_fd);
        glBindFramebuffer(GL_FRAMEBUFFER, out_framebuffer);
        GLERROR();

        glViewport(0, 0, m_width, m_height);
        GLERROR();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        GLERROR();

        glUseProgram(m_programs[4]);
        GLERROR();

        glUniform1i(glGetUniformLocation(m_programs[4], "tex"), 0);
        GLERROR();
        glUniform1i(glGetUniformLocation(m_programs[4], "tiles"), 1);
        GLERROR();

        glActiveTexture(GL_TEXTURE0);
        GLERROR();
        glBindTexture(GL_TEXTURE_2D, m_grayscale_texture);
        GLERROR();

        glActiveTexture(GL_TEXTURE1);
        GLERROR();
        glBindTexture(GL_TEXTURE_2D, m_min_max_texture);
        GLERROR();

        auto tile_res = glGetUniformLocation(m_programs[4], "tile_resolution");
        GLERROR();
        glUniform2f(tile_res, m_width / 4, m_height / 4);
        GLERROR();

        glDrawArrays(GL_TRIANGLES, 0, 6);
        GLERROR();

        glFinish();
        GLERROR();

        return framebuffer_fd;
    }
}

void GlHsvThresholder::returnBuffer(int fd) {
    std::scoped_lock lock(m_renderable_mutex);
    m_renderable.push(fd);
}

void GlHsvThresholder::setHsvThresholds(double hl, double sl, double vl,
                                        double hu, double su, double vu,
                                        bool hueInverted) {
    std::lock_guard lock{m_hsv_mutex};
    m_hsvLower[0] = hl;
    m_hsvLower[1] = sl;
    m_hsvLower[2] = vl;

    m_hsvUpper[0] = hu;
    m_hsvUpper[1] = su;
    m_hsvUpper[2] = vu;
    m_invertHue = hueInverted;
}
