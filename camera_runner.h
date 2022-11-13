#pragma once

#include "blocking_future.h"
#include "camera_grabber.h"
#include "concurrent_blocking_queue.h"
#include "dma_buf_alloc.h"
#include "gl_hsv_thresholder.h"
#include "libcamera_opengl_utility.h"

#include <atomic>
#include <libcamera/camera.h>
#include <memory>
#include <string>
#include <thread>

#include <opencv2/core.hpp>

struct MatPair {
    std::unique_ptr<cv::Mat> color;
    std::unique_ptr<cv::Mat> processed;

    MatPair() = default;
    explicit MatPair(int width, int height)
        : color(std::make_unique<cv::Mat>(height, width, CV_8UC3)),
          processed(std::make_unique<cv::Mat>(height, width, CV_8UC1)) {}
};

// Note: destructing this class without calling `stop` if `start` was called
// is undefined behavior.
class CameraRunner {
  public:
    CameraRunner(int width, int height, int fps,
                 std::shared_ptr<libcamera::Camera> cam);
    ~CameraRunner();

    inline CameraGrabber &cameraGrabber() { return grabber; }
    inline GlHsvThresholder &thresholder() { return m_thresholder; }
    inline const std::string &model() { return m_model; }

    // Note: all following functions must be protected by mutual exclusion.
    // Failure to do so will result in UB.

    // Note: start and stop are not reenterant. Starting and stopping a camera
    // repeatedly should work, but has not been thoroughly tested.
    void start();
    void stop();

    // Note: this is public but is a footgun. Destructing this class while a
    // thread is blocked on this waiting for a frame is UB.
    // TODO: consider making this a shared pointer to remove this footgun
    BlockingFuture<MatPair> outgoing;

  private:
    std::thread m_threshold;
    std::shared_ptr<libcamera::Camera> m_camera;
    int m_width, m_height, m_fps;

    CameraGrabber grabber;

    ConcurrentBlockingQueue<libcamera::Request *> camera_queue{};
    ConcurrentBlockingQueue<int> gpu_queue{};
    GlHsvThresholder m_thresholder;
    DmaBufAlloc allocer;

    std::vector<int> fds{};

    std::mutex camera_stop_mutex;

    std::thread threshold;
    std::thread display;

    std::string m_model;
    int32_t m_rotation = 0;
};
