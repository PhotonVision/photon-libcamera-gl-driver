#pragma once

#include <libcamera/camera.h>
#include <libcamera/framebuffer_allocator.h>

#include "camera_model.h"

#include <functional>
#include <memory>
#include <optional>

struct CameraSettings {
    int32_t exposureTimeUs = 10000;
    float analogGain = 2;
    float brightness = 0.0;
    float contrast = 1;
    float awbRedGain = 1.5;
    float awbBlueGain = 1.5;
    float saturation = 1;
    bool doAutoExposure = false;
    bool doAutofocus = false;
};

class CameraGrabber {
  public:
    explicit CameraGrabber(std::shared_ptr<libcamera::Camera> camera, int width,
                           int height, int rotation);
    ~CameraGrabber();

    const libcamera::StreamConfiguration &streamConfiguration();
    void setOnData(std::function<void(libcamera::Request *)> onData);
    void resetOnData();

    inline const CameraModel model() { return m_model; }

    inline CameraSettings &cameraSettings() { return m_settings; }

    int getAutofocusStatus();

    // Note: these 3 functions must be protected by mutual exclusion.
    // Failure to do so will result in UB.
    void startAndQueue();
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

    int afState;

    CameraSettings m_settings{};
    bool running = false;

    void setControls(libcamera::Request *request);
};
