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
    cv::Mat color;
    cv::Mat processed;
    long captureTimestamp; // In libcamera time units, hopefully uS? TODO actually implement
    int32_t frameProcessingType; // enum value of shader run on the image

    MatPair() = default;
    explicit MatPair(int width, int height)
        : color(height, width, CV_8UC3),
          processed(height, width, CV_8UC1) {}
};

// Note: destructing this class without calling `stop` if `start` was called
// is undefined behavior.
class CameraRunner {
  public:
    CameraRunner(int width, int height, int rotation,
                 std::shared_ptr<libcamera::Camera> cam);
    ~CameraRunner();

    inline CameraGrabber &cameraGrabber() { return grabber; }
    inline GlHsvThresholder &thresholder() { return m_thresholder; }
    inline CameraModel model() const { return grabber.model(); }
    void setCopyOptions(bool copyInput, bool copyOutput);

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

    void requestShaderIdx(int idx);


  private:

    struct GpuQueueData {
        int fd;
        ProcessType type;
        uint64_t captureTimestamp;
    };

    std::thread m_threshold;
    std::shared_ptr<libcamera::Camera> m_camera;
    int m_width, m_height;

    CameraGrabber grabber;
    ConcurrentBlockingQueue<libcamera::Request *> camera_queue{};
    ConcurrentBlockingQueue<GpuQueueData> gpu_queue{};
    GlHsvThresholder m_thresholder;
    DmaBufAlloc allocer;

    std::vector<int> fds{};

    std::mutex camera_stop_mutex;

    std::thread threshold;
    std::thread display;


    std::atomic<int> m_shaderIdx = 0;

    std::atomic<bool> m_copyInput;
    std::atomic<bool> m_copyOutput;
};
