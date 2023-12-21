#pragma once

#include <EGL/egl.h>

#ifdef __cplusplus
extern "C" {
#endif

struct HeadlessData {
    int gbmFd;
    struct gbm_device *gbmDevice;

    EGLDisplay display;
    EGLContext context;
};

struct HeadlessData createHeadless();
void destroyHeadless(struct HeadlessData status);

#ifdef __cplusplus
}
#endif