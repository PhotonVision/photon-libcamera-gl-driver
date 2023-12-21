#include "libcamera_opengl_utility.h"

#include <EGL/eglext.h>

#include <stdexcept>

EGLint rangeFromColorspace(const libcamera::ColorSpace &colorSpace) {
    if (colorSpace.range == libcamera::ColorSpace::Range::Full) {
        return EGL_YUV_FULL_RANGE_EXT;
    } else {
        return EGL_YUV_NARROW_RANGE_EXT;
    }
}

EGLint encodingFromColorspace(const libcamera::ColorSpace &colorSpace) {
    if (colorSpace.ycbcrEncoding ==
        libcamera::ColorSpace::YcbcrEncoding::Rec2020) {
        return EGL_ITU_REC2020_EXT;
    } else if (colorSpace.ycbcrEncoding ==
               libcamera::ColorSpace::YcbcrEncoding::Rec709) {
        return EGL_ITU_REC709_EXT;
    } else if (colorSpace.ycbcrEncoding ==
               libcamera::ColorSpace::YcbcrEncoding::Rec601) {
        return EGL_ITU_REC601_EXT;
    } else {
        throw std::runtime_error("unknown color space encoding");
    }
}
