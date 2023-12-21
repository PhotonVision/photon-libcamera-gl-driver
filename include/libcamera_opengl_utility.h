#pragma once

// These headers are weird for some reason

// clang-format off
#include <libcamera/color_space.h>
#include <EGL/egl.h>
// clang-format on

EGLint rangeFromColorspace(const libcamera::ColorSpace &colorSpace);
EGLint encodingFromColorspace(const libcamera::ColorSpace &colorSpace);
