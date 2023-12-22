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

#include "headless_opengl.h"

#include <fcntl.h>
#include <gbm.h>
#include <unistd.h>

#include <vector>

#include <EGL/eglext.h>

#include "glerror.h"

// The following code related to DRM/GBM was adapted from the following sources:
// https://github.com/eyelash/tutorials/blob/master/drm-gbm.c
// and
// https://www.raspberrypi.org/forums/viewtopic.php?t=243707#p1499181
//
// I am not the original author of this code, I have only modified it.
static int matchConfigToVisual(EGLDisplay display, EGLint visualId,
                               EGLConfig *configs, int count) {
    EGLint id;
    for (int i = 0; i < count; ++i) {
        if (!eglGetConfigAttrib(display, configs[i], EGL_NATIVE_VISUAL_ID, &id))
            continue;
        if (id == visualId)
            return i;
    }
    return -1;
}

static const EGLint configAttribs[] = {EGL_RED_SIZE,
                                       8,
                                       EGL_GREEN_SIZE,
                                       8,
                                       EGL_BLUE_SIZE,
                                       8,
                                       EGL_DEPTH_SIZE,
                                       8,
                                       EGL_RENDERABLE_TYPE,
                                       EGL_OPENGL_ES2_BIT,
                                       EGL_NONE};

static const EGLint contextAttribs[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2, EGL_CONTEXT_FLAGS_KHR,
    EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR, EGL_NONE};

HeadlessData createHeadless() {
    std::vector<std::string> paths = {"/dev/dri/card1", "/dev/dri/card0"};
    int device = -1;
    for (const auto &path : paths) {
        device = open(path.c_str(), O_RDWR | O_CLOEXEC);
        if (device != -1) {
            break;
        }
    }

    // no device could be opened
    if (device == -1) {
        throw std::runtime_error("Unable to open graphics fd");
    }

    gbm_device *gbmDevice = gbm_create_device(device);
    if (gbmDevice == nullptr) {
        throw std::runtime_error("Unable to create GBM device");
    }

    EGLDisplay display = eglGetDisplay((EGLNativeDisplayType)gbmDevice);
    if (display == EGL_NO_DISPLAY) {
        gbm_device_destroy(gbmDevice);
        close(device);
        throw std::runtime_error("Unable to get EGL display");
    }

    int major, minor;
    if (eglInitialize(display, &major, &minor) == EGL_FALSE) {
        eglTerminate(display);
        gbm_device_destroy(gbmDevice);
        close(device);
        EGLERROR();
    }

    eglBindAPI(EGL_OPENGL_ES_API);
    EGLERROR();

    std::printf("Initialized EGL version: %d.%d\n", major, minor);

    EGLint count;
    EGLint numConfigs;
    eglGetConfigs(display, nullptr, 0, &count);
    EGLERROR();
    auto *configs = new EGLConfig[count];

    if (!eglChooseConfig(display, configAttribs, configs, count, &numConfigs)) {
        eglTerminate(display);
        gbm_device_destroy(gbmDevice);
        close(device);
        EGLERROR();
    }

    // I am not exactly sure why the EGL config must match the GBM format.
    // But it works!
    int configIndex =
        matchConfigToVisual(display, GBM_FORMAT_XRGB8888, configs, numConfigs);
    if (configIndex < 0) {
        eglTerminate(display);
        gbm_device_destroy(gbmDevice);
        close(device);
        EGLERROR();
    }

    EGLContext context = eglCreateContext(display, configs[configIndex],
                                          EGL_NO_CONTEXT, contextAttribs);
    if (context == EGL_NO_CONTEXT) {
        eglTerminate(display);
        gbm_device_destroy(gbmDevice);
        close(device);
        EGLERROR();
    }

    delete[] configs;

    HeadlessData ret{.gbmFd = device,
                     .gbmDevice = gbmDevice,
                     .display = display,
                     .context = context};

    return ret;
}

#include <iostream>
void destroyHeadless(HeadlessData status) {
    std::cout << "Destroying headless" << std::endl;
    eglDestroyContext(status.display, status.context);
    eglTerminate(status.display);
    gbm_device_destroy(status.gbmDevice);
    close(status.gbmFd);
}
