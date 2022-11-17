#pragma once

#include <array>
#include <cstdint>
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

#include "headless_opengl.h"

enum class ProcessType: int32_t {
    None,
    Hsv,
    Gray,
    Adaptive,
};

class GlHsvThresholder {
  public:
    struct DmaBufPlaneData {
        int fd;
        EGLint offset;
        EGLint pitch;
    };

    explicit GlHsvThresholder(int width, int height);
    ~GlHsvThresholder();

    void start(const std::vector<int> &output_buf_fds);

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
     */
    void setHsvThresholds(double hl, double sl, double vl, double hu, double su,
                          double vu);


  private:
    int m_width;
    int m_height;

    std::unordered_map<int, GLuint> m_framebuffers; // (dma_buf fd, framebuffer)
    std::queue<int> m_renderable;
    std::mutex m_renderable_mutex;

    GLuint m_quad_vbo = 0;
    std::vector<GLuint> m_programs = {};

    HeadlessData m_status{};
    EGLDisplay m_display;
    EGLContext m_context;

    std::mutex m_hsv_mutex;
    double m_hsvLower[3] = {0}; // Hue, sat, value, in [0,1]
    double m_hsvUpper[3] = {0}; // Hue, sat, value, in [0,1]
};
