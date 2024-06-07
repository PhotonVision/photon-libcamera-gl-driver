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

#include <libcamera/camera.h>
#include <libcamera/framebuffer_allocator.h>

#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include "camera_model.h"

struct CameraSettings {
    int32_t exposureTimeUs = 10000;
    float analogGain = 2;
    float brightness = 0.0;
    float contrast = 1;
    float awbRedGain = 1.5;
    float awbBlueGain = 1.5;
    float saturation = 1;
    bool doAutoExposure = false;
    // float digitalGain = 100;
};

class CameraGrabber {
  public:
    explicit CameraGrabber(std::shared_ptr<libcamera::Camera> camera, int width,
                           int height, int rotation);
    ~CameraGrabber();

    const libcamera::StreamConfiguration &streamConfiguration();
    void setOnData(std::function<void(libcamera::Request *)> onData);
    void resetOnData();

    inline CameraModel model() const { return m_model; }

    inline CameraSettings &cameraSettings() { return m_settings; }

    // Note: these 3 functions must be protected by mutual exclusion.
    // Failure to do so will result in UB.
    bool startAndQueue();
    void stop();
    void requeueRequest(libcamera::Request *request);

  private:
    void requestComplete(libcamera::Request *request);

    // The `FrameBufferAllocator` must be first here as it must be
    // destroyed last, or else we get fun UAFs for some reason...
    libcamera::FrameBufferAllocator m_buf_allocator;
    std::vector<std::unique_ptr<libcamera::Request>> m_requests;
    std::shared_ptr<libcamera::Camera> m_camera;
    CameraModel m_model;
    std::optional<std::array<libcamera::ControlValue, 4>>
        m_cameraExposureProfiles;
    std::unique_ptr<libcamera::CameraConfiguration> m_config;

    std::optional<std::function<void(libcamera::Request *)>> m_onData;

    CameraSettings m_settings{};
    bool running = false;

    void setControls(libcamera::Request *request);
};
