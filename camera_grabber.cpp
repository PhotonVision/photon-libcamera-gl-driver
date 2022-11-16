#include "camera_grabber.h"

#include <iostream>
#include <stdexcept>

#include <libcamera/control_ids.h>

CameraGrabber::CameraGrabber(std::shared_ptr<libcamera::Camera> camera,
                             int width, int height)
    : m_buf_allocator(camera), m_camera(std::move(camera)) {
    if (m_camera->acquire()) {
        throw std::runtime_error("failed to acquire camera");
    }

    auto config = m_camera->generateConfiguration(
        {libcamera::StreamRole::VideoRecording});
    config->at(0).size.width = width;
    config->at(0).size.height = height;
    config->transform = libcamera::Transform::Identity;

    if (config->validate() == libcamera::CameraConfiguration::Invalid) {
        throw std::runtime_error("failed to validate config");
    }

    if (m_camera->configure(config.get()) < 0) {
        throw std::runtime_error("failed to configure stream");
    }

    std::cout << config->at(0).toString() << std::endl;

    auto stream = config->at(0).stream();
    if (m_buf_allocator.allocate(stream) < 0) {
        throw std::runtime_error("failed to allocate buffers");
    }
    m_config = std::move(config);

    for (const auto &buffer : m_buf_allocator.buffers(stream)) {
        auto request = m_camera->createRequest();

        auto &controls = request->controls();
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
    controls_.set(libcamera::controls::AeEnable,
                 false); // Auto exposure disabled
    controls_.set(libcamera::controls::AwbEnable, false); // AWB disabled
    controls_.set(libcamera::controls::ExposureTime,
                 m_settings.exposureTimeUs); // in microseconds
    controls_.set(libcamera::controls::AnalogueGain,
                 m_settings.analogGain); // Analog gain, min 1 max big number?
    controls_.set(libcamera::controls::ColourGains,
                 libcamera::Span<const float, 2>{
                     {m_settings.awbRedGain,
                      m_settings.awbBlueGain}}); // AWB gains, red and blue,
                                                 // unknown range
    // controls_.set(libcamera::controls::Brightness,
    //              m_settings.brightness); // -1 to 1, 0 means unchanged
    controls_.set(libcamera::controls::Contrast,
                 m_settings.contrast); // Nominal 1
    controls_.set(libcamera::controls::Saturation,
                 m_settings.saturation); // Nominal 1, 0 would be greyscale
    controls_.set(
        libcamera::controls::FrameDurationLimits,
        libcamera::Span<const int64_t, 2>{
            {m_settings.exposureTimeUs,
             m_settings.exposureTimeUs}}); // Set default to zero, we have
                                           // specified the exposure time

    // Additionally, we can set crop regions and stuff
    // controls.set(libcamera::controls::DigitalGain, m_settings.digitalGain);
    // // Digital gain, unknown range

	// Framerate is a bit weird. If it was set programmatically, we go with that, but
	// otherwise it applies only to preview/video modes. For stills capture we set it
	// as long as possible so that we get whatever the exposure profile wants.

	// if (!controls_.get(controls::FrameDurationLimits))
	// {
	// 	// if (StillStream())
	// 	// 	controls_.set(controls::FrameDurationLimits,
	// 	// 				  libcamera::Span<const int64_t, 2>({ INT64_C(100), INT64_C(1000000000) }));
	// 	// else if (!options_->framerate || options_->framerate.value() > 0)
	// 	// {
	// 	// 	int64_t frame_time = 1000000 / options_->framerate.value_or(DEFAULT_FRAMERATE); // in us

    //         auto frame_time = 10000;
	// 		controls_.set(controls::FrameDurationLimits,
	// 					  libcamera::Span<const int64_t, 2>({ frame_time, frame_time }));
	// 	// }
	// }

	// if (!controls_.get(controls::ExposureTime) && options_->shutter)
	// 	controls_.set(controls::ExposureTime, options_->shutter);
	// if (!controls_.get(controls::AnalogueGain) && options_->gain)
	// 	controls_.set(controls::AnalogueGain, options_->gain);
	// if (!controls_.get(controls::AeMeteringMode))
	// 	controls_.set(controls::AeMeteringMode, 0);
	if (!controls_.get(controls::AeExposureMode))
		controls_.set(controls::AeExposureMode, 0);
	if (!controls_.get(controls::ExposureValue))
		controls_.set(controls::ExposureValue, 0);
	// if (!controls_.get(controls::AwbMode))
	// 	controls_.set(controls::AwbMode, 0);
	// if (!controls_.get(controls::ColourGains) && options_->awb_gain_r && options_->awb_gain_b)
	// 	controls_.set(controls::ColourGains,
	// 				  libcamera::Span<const float, 2>({ options_->awb_gain_r, options_->awb_gain_b }));
	if (!controls_.get(controls::Brightness))
		controls_.set(controls::Brightness, 0);
	// if (!controls_.get(controls::Contrast))
	// 	controls_.set(controls::Contrast, 1);
	// if (!controls_.get(controls::Saturation))
	// 	controls_.set(controls::Saturation, 1);
	if (!controls_.get(controls::Sharpness))
		controls_.set(controls::Sharpness, 1);
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
