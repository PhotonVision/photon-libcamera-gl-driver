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

#include "libcamera_opengl_utility.h"

#include <stdexcept>

#include <EGL/eglext.h>

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
