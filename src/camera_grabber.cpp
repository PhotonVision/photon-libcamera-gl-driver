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

#include "camera_grabber.h"

#include <libcamera/control_ids.h>
#include <libcamera/property_ids.h>

#include <iostream>
#include <stdexcept>

CameraGrabber::CameraGrabber(std::shared_ptr<libcamera::Camera> camera,
                             int width, int height, int rotation)
    : m_buf_allocator(camera), m_camera(std::move(camera)),
      m_cameraExposureProfiles(std::nullopt) {

    if (m_camera->acquire()) {
        throw std::runtime_error("failed to acquire camera");
    }

    // Determine model
    auto &cprp = m_camera->properties();
    auto model = cprp.get(libcamera::properties::Model);
    if (model) {
        m_model = stringToModel(model.value());
    } else {
        m_model = Unknown;
    }

    std::cout << "Model " << m_model << std::endl;

    auto config = m_camera->generateConfiguration(
        {libcamera::StreamRole::VideoRecording});

    // print active arrays
    if (m_camera->properties().contains(
            libcamera::properties::PIXEL_ARRAY_ACTIVE_AREAS)) {
        std::printf("Active areas:\n");
        auto rects = m_camera->properties().get(
            libcamera::properties::PixelArrayActiveAreas);
        if (rects.has_value()) {
            for (const auto rect : rects.value()) {
                std::cout << rect.toString() << std::endl;
            }
        }
    } else {
        std::printf("No active areas??\n");
    }

    config->at(0).size.width = width;
    config->at(0).size.height = height;

    std::printf("Rotation = %i\n", rotation);
    if (rotation == 180) {
        config->orientation = libcamera::Orientation::Rotate180;
    } else {
        config->orientation = libcamera::Orientation::Rotate0;
    }

    if (config->validate() == libcamera::CameraConfiguration::Invalid) {
        throw std::runtime_error("failed to validate config");
    }

    if (m_camera->configure(config.get()) < 0) {
        throw std::runtime_error("failed to configure stream");
    }

    std::cout << "Selected configuration: " << config->at(0).toString() << std::endl;

    auto stream = config->at(0).stream();
    if (m_buf_allocator.allocate(stream) < 0) {
        throw std::runtime_error("failed to allocate buffers");
    }
    m_config = std::move(config);

    for (const auto &buffer : m_buf_allocator.buffers(stream)) {
        auto request = m_camera->createRequest();

        // auto &controls = request->controls();
        // controls.set(libcamera::controls::FrameDurationLimits,
        // {static_cast<int64_t>(8333), static_cast<int64_t>(8333)});
        // controls.set(libcamera::controls::ExposureTime, 10000);

        request->addBuffer(stream, buffer.get());
        m_requests.push_back(std::move(request));
    }

    m_camera->requestCompleted.connect(this, &CameraGrabber::requestComplete);
}

CameraGrabber::~CameraGrabber() {
    m_camera->release();
    m_camera->requestCompleted.disconnect(this,
                                          &CameraGrabber::requestComplete);
}

void CameraGrabber::requestComplete(libcamera::Request *request) {
    if (request->status() == libcamera::Request::RequestCancelled) {
        return;
    }

    static int i = 0;

    i++;

    if (m_onData) {
        m_onData->operator()(request);
    }
}

void CameraGrabber::requeueRequest(libcamera::Request *request) {
    if (running) {
        // This resets all our controls
        // https://github.com/kbingham/libcamera/blob/master/src/libcamera/request.cpp#L397
        request->reuse(libcamera::Request::ReuseFlag::ReuseBuffers);

        setControls(request);

        if (m_camera->queueRequest(request) < 0) {
            throw std::runtime_error("failed to queue request");
        }
    }
}

void CameraGrabber::setControls(libcamera::Request *request) {
    using namespace libcamera;

    auto &controls_ = request->controls();
    if (m_model != OV9281) {
        controls_.set(controls::AwbEnable, false); // AWB disabled
    }
    controls_.set(controls::AnalogueGain,
                  m_settings.analogGain); // Analog gain, min 1 max big number?

    if (m_model != OV9281) {
        controls_.set(controls::ColourGains,
                      libcamera::Span<const float, 2>{
                          {m_settings.awbRedGain,
                           m_settings.awbBlueGain}}); // AWB gains, red and
                                                      // blue, unknown range
    }

    // Note about brightness: -1 makes everything look deep fried, 0 is probably
    // best for most things
    controls_.set(libcamera::controls::Brightness,
                  m_settings.brightness); // -1 to 1, 0 means unchanged
    controls_.set(controls::Contrast,
                  m_settings.contrast); // Nominal 1

    if (m_model != OV9281) {
        controls_.set(controls::Saturation,
                      m_settings.saturation); // Nominal 1, 0 would be greyscale
    }

    if (m_settings.doAutoExposure) {
        controls_.set(controls::AeEnable,
                      true); // Auto exposure disabled

        controls_.set(controls::AeMeteringMode,
                      controls::MeteringCentreWeighted);
        if (m_model == OV9281) {
            controls_.set(controls::AeExposureMode, controls::ExposureNormal);
        } else {
            controls_.set(controls::AeExposureMode, controls::ExposureShort);
        }

        // 1/fps=seconds
        // seconds * 1e6 = uS
        constexpr const int MIN_FRAME_TIME = 1e6 / 250;
        constexpr const int MAX_FRAME_TIME = 1e6 / 15;
        controls_.set(libcamera::controls::FrameDurationLimits,
                      libcamera::Span<const int64_t, 2>{
                          {MIN_FRAME_TIME, MAX_FRAME_TIME}});
    } else {
        controls_.set(controls::AeEnable,
                      false); // Auto exposure disabled
        controls_.set(controls::ExposureTime,
                      m_settings.exposureTimeUs); // in microseconds
        controls_.set(
            libcamera::controls::FrameDurationLimits,
            libcamera::Span<const int64_t, 2>{
                {m_settings.exposureTimeUs,
                 m_settings.exposureTimeUs}}); // Set default to zero, we have
                                               // specified the exposure time
    }

    controls_.set(controls::ExposureValue, 0);

    if (m_model != OV7251 && m_model != OV9281) {
        controls_.set(controls::Sharpness, 1);
    }
}

void CameraGrabber::startAndQueue() {
    running = true;
    if (m_camera->start()) {
        throw std::runtime_error("failed to start camera");
    }

    // TODO: HANDLE THIS BETTER
    for (auto &request : m_requests) {
        setControls(request.get());
        if (m_camera->queueRequest(request.get()) < 0) {
            throw std::runtime_error("failed to queue request");
        }
    }
}

void CameraGrabber::stop() {
    running = false;
    m_camera->stop();
}

void CameraGrabber::setOnData(
    std::function<void(libcamera::Request *)> onData) {
    m_onData = std::move(onData);
}

void CameraGrabber::resetOnData() { m_onData.reset(); }

const libcamera::StreamConfiguration &CameraGrabber::streamConfiguration() {
    return m_config->at(0);
}
