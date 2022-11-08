#pragma once

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

#include "headless_opengl.h"

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
    void setOnComplete(std::function<void(int)> onComplete);
    void resetOnComplete();

    void returnBuffer(int fd);
    void testFrame(
        const std::array<GlHsvThresholder::DmaBufPlaneData, 3> &yuv_plane_data,
        EGLint encoding, EGLint range);

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
    std::optional<std::function<void(int)>> m_onComplete;

    EGLDisplay m_display;
    EGLContext m_context;

    std::unordered_map<int, GLuint> m_framebuffers; // (dma_buf fd, framebuffer)
    std::queue<int> m_renderable;
    std::mutex m_renderable_mutex;

    GLuint m_quad_vbo;
    GLuint m_program;

    HeadlessData status;

    double m_hsvLower[3] = {0}; // Hue, sat, value, in [0,1]
    double m_hsvUpper[3] = {0}; // Hue, sat, value, in [0,1]
};
