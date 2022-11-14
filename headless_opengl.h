#pragma once

#include <EGL/egl.h>

struct HeadlessData {
    int gbmFd;
    struct gbm_device *gbmDevice;

    EGLDisplay display;
    EGLContext context;
};

#ifdef __cplusplus
extern "C" {
#endif

struct HeadlessData createHeadless();
void destroyHeadless(struct HeadlessData status);

#ifdef __cplusplus
}
#endif