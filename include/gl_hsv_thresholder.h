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

#include <stdint.h>

#include <array>
#include <functional>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "camera_model.h"

#include "headless_opengl.h"


enum class ProcessType : int32_t {
    None = 0,
    Hsv,
    Gray,
    Adaptive,
    NUM_PROCESS_TYPES
};

class GlHsvThresholder {
  public:
    struct DmaBufPlaneData {
        int fd;
        EGLint offset;
        EGLint pitch;
    };

    explicit GlHsvThresholder(int width, int height, CameraModel model);
    ~GlHsvThresholder();

    void start(const std::vector<int> &output_buf_fds);
    void release();

    void returnBuffer(int fd);
    int testFrame(
        const std::array<GlHsvThresholder::DmaBufPlaneData, 3> &yuv_plane_data,
        EGLint encoding, EGLint range, ProcessType type);

    /**
     * @brief Set the Hsv Thresholds range, on [0..1]
     *
     * @param hl Lower hue
     * @param sl Sat lower
     * @param vl Value lower
     * @param hu Upper hue
     * @param su Sat Upper
     * @param vu Value Upper
     * @param hueInverted if the range is [hl, hu] or [hu..180] | [0..hl]
     */
    void setHsvThresholds(double hl, double sl, double vl, double hu, double su,
                          double vu, bool hueInverted);

  private:
    int m_width;
    int m_height;
    bool useGrayScalePassThrough;

    std::unordered_map<int, GLuint> m_framebuffers; // (dma_buf fd, framebuffer)
    std::queue<int> m_renderable;
    std::mutex m_renderable_mutex;

    GLuint m_quad_vbo = 0;
    GLuint m_grayscale_texture = 0;
    GLuint m_grayscale_buffer = 0;
    GLuint m_min_max_texture = 0;
    GLuint m_min_max_framebuffer = 0;
    std::vector<GLuint> m_programs = {};

    HeadlessData m_status;
    EGLDisplay m_display;
    EGLContext m_context;

    std::mutex m_hsv_mutex;
    double m_hsvLower[3] = {0}; // Hue, sat, value, in [0,1]
    double m_hsvUpper[3] = {0}; // Hue, sat, value, in [0,1]
    bool m_invertHue;
};
