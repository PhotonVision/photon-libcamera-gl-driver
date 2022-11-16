#include "gl_hsv_thresholder.h"
#include "glerror.h"

#include <EGL/eglext.h>
#include <GLES2/gl2ext.h>
#include <libdrm/drm_fourcc.h>

#include <iostream>
#include <stdexcept>

#define GLERROR() glerror(__LINE__)
#define EGLERROR() eglerror(__LINE__)

static constexpr const char *VERTEX_SOURCE =
    "#version 100\n"
    ""
    "attribute vec2 vertex;"
    "varying vec2 texcoord;"
    ""
    "void main(void) {"
    "   texcoord = 0.5 * (vertex + 1.0);"
    "   gl_Position = vec4(vertex, 0.0, 1.0);"
    "}";

static constexpr const char *HSV_FRAGMENT_SOURCE =
    "#version 100\n"
    "#extension GL_OES_EGL_image_external : require\n"
    ""
    "precision lowp float;"
    "precision lowp int;"
    ""
    "varying vec2 texcoord;"
    ""
    "uniform vec3 lowerThresh;"
    "uniform vec3 upperThresh;"
    "uniform samplerExternalOES tex;"
    ""
    "vec3 rgb2hsv(const vec3 p) {"
    "  const vec4 H = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);"
    // Using ternary seems to be faster than using mix and step
    "  vec4 o = mix(vec4(p.bg, H.wz), vec4(p.gb, H.xy), step(p.b, p.g));"
    "  vec4 t = mix(vec4(o.xyw, p.r), vec4(p.r, o.yzx), step(o.x, p.r));"
    ""
    "  float O = t.x - min(t.w, t.y);"
    "  const float n = 1.0e-10;"
    "  return vec3(abs(t.z + (t.w - t.y) / (6.0 * O + n)), O / (t.x + n), "
    "t.x);"
    "}"
    ""
    "bool inRange(vec3 hsv) {"
    "  const float epsilon = 0.0001;"
    "  bvec3 botBool = greaterThanEqual(hsv, lowerThresh - epsilon);"
    "  bvec3 topBool = lessThanEqual(hsv, upperThresh + epsilon);"
    "  return all(botBool) && all(topBool);"
    "}"
    ""
    "void main(void) {"
    "  vec3 col = texture2D(tex, texcoord).rgb;"
    "  gl_FragColor = vec4(col.bgr, int(inRange(rgb2hsv(col))));"
    "}";

static constexpr const char *GRAY_FRAGMENT_SOURCE =
    "#version 100\n"
    "#extension GL_OES_EGL_image_external : require\n"
    ""
    "precision lowp float;"
    "precision lowp int;"
    ""
    "varying vec2 texcoord;"
    ""
    "uniform samplerExternalOES tex;"
    ""
    "void main(void) {"
    "    vec3 gammaColor = texture2D(tex, texcoord.xy).rgb;"
    "    vec3 color = pow(gammaColor, vec3(2.0));"
    "    float gray = dot(color, vec3(0.2126, 0.7152, 0.0722));"
    "    float gammaGray = sqrt(gray);"
    "    gl_FragColor = vec4(color.bgr, gammaGray);"
    "}";

GLuint make_shader(GLenum type, const char *source) {
    auto shader = glCreateShader(type);
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
    status = createHeadless();
    m_context = status.context;
    m_display = status.display;
}

GlHsvThresholder::~GlHsvThresholder() {
    glDeleteProgram(m_program);
    glDeleteBuffers(1, &m_quad_vbo);
    for (const auto [key, value]: m_framebuffers) {
        glDeleteFramebuffers(1, &value);
    }
    destroyHeadless(status);
}

void GlHsvThresholder::setShaderProgramIdx(int idx) {
    if (idx != m_lastShaderIdx) {
        printf("Setting shader to idx %i\n", idx);

        m_programs[0] = make_program(VERTEX_SOURCE, HSV_FRAGMENT_SOURCE);
        m_programs[1] = make_program(VERTEX_SOURCE, GRAY_FRAGMENT_SOURCE);

        auto program = m_programs[idx];

        glUseProgram(program);
        GLERROR();
        glUniform1i(glGetUniformLocation(program, "tex"), 0);
        GLERROR();

        m_lastShaderIdx = idx;
    }
}

void GlHsvThresholder::start(const std::vector<int> &output_buf_fds) {
    static auto glEGLImageTargetTexture2DOES =
        (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress(
            "glEGLImageTargetTexture2DOES");
    static auto eglCreateImageKHR =
        (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");

    if (!eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, m_context)) {
        throw std::runtime_error("failed to bind egl context");
    }
    EGLERROR();

    setShaderProgramIdx(0);

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
}

int GlHsvThresholder::testFrame(
    const std::array<GlHsvThresholder::DmaBufPlaneData, 3> &yuv_plane_data,
    EGLint encoding, EGLint range, int shaderIdx) {
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
            std::cout << "no framebuffer, skipping" << std::endl;
            return 0;
        }
    }

    setShaderProgramIdx(shaderIdx);

    auto framebuffer = m_framebuffers.at(framebuffer_fd);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    GLERROR();

    // Set GL Viewport size, always needed!
    glViewport(0, 0, m_width, m_height);

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

    EGLERROR();
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
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLERROR();
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    GLERROR();
    glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, image);
    GLERROR();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    GLERROR();

    glUseProgram(m_program);
    GLERROR();

    glActiveTexture(GL_TEXTURE0);
    GLERROR();
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture);
    GLERROR();

    glBindBuffer(GL_ARRAY_BUFFER, m_quad_vbo);
    GLERROR();
    // TODO: refactor these
    auto attr_loc = glGetAttribLocation(m_program, "vertex");
    glEnableVertexAttribArray(attr_loc);
    GLERROR();
    glVertexAttribPointer(attr_loc, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    GLERROR();
    auto lll = glGetUniformLocation(m_program, "lowerThresh");
    glUniform3f(lll, m_hsvLower[0], m_hsvLower[1], m_hsvLower[3]);
    GLERROR();
    auto uuu = glGetUniformLocation(m_program, "upperThresh");
    glUniform3f(uuu, m_hsvUpper[0], m_hsvUpper[1], m_hsvUpper[3]);
    GLERROR();

    glDrawArrays(GL_TRIANGLES, 0, 6);
    GLERROR();

    glFinish();
    GLERROR();

    eglDestroyImageKHR(m_display, image);
    EGLERROR();
    glDeleteTextures(1, &texture);
    GLERROR();

    return framebuffer_fd;
}

void GlHsvThresholder::returnBuffer(int fd) {
    std::scoped_lock lock(m_renderable_mutex);
    m_renderable.push(fd);
}

void GlHsvThresholder::setHsvThresholds(double hl, double sl, double vl,
                                        double hu, double su, double vu) {
    m_hsvLower[0] = hl;
    m_hsvLower[1] = sl;
    m_hsvLower[2] = vl;

    m_hsvUpper[0] = hu;
    m_hsvUpper[1] = su;
    m_hsvUpper[2] = vu;
}
