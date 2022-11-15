#include "camera_runner.h"

#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

#ifdef __cpp_lib_latch
#include <latch>
using latch = std::latch;
#else
#include "latch.hpp"
using latch = Latch;
#endif

#include <sys/mman.h>
#include <unistd.h>

#include <libcamera/property_ids.h>

using namespace std::chrono;
using namespace std::chrono_literals;

#include <opencv2/imgcodecs.hpp>

static double approxRollingAverage(double avg, double new_sample) {
    avg -= avg / 50;
    avg += new_sample / 50;

    return avg;
}

CameraRunner::CameraRunner(int width, int height, int fps,
                           std::shared_ptr<libcamera::Camera> cam)
    : m_camera(std::move(cam)), m_width(width), m_height(height), m_fps(fps),
      grabber(m_camera, m_width, m_height), m_thresholder(m_width, m_height),
      allocer("/dev/dma_heap/linux,cma") {

    auto &cprp = m_camera->properties();
    auto model = cprp.get(libcamera::properties::Model);
    if (model) {
        m_model = std::move(model.value());
    } else {
        m_model = "No Camera Found";
    }

    std::cout << "Model " << m_model << " rot " << m_rotation << std::endl;

    grabber.setOnData(
        [&](libcamera::Request *request) { camera_queue.push(request); });

    fds = {allocer.alloc_buf_fd(m_width * m_height * 4),
           allocer.alloc_buf_fd(m_width * m_height * 4),
           allocer.alloc_buf_fd(m_width * m_height * 4)};
}

CameraRunner::~CameraRunner() {
    for (auto i : fds) {
        close(i);
    }
}

void CameraRunner::start() {
    unsigned int stride = grabber.streamConfiguration().stride;

    latch start_frame_grabber{2};

    threshold = std::thread([&, stride]() {
        m_thresholder.start(fds);
        auto colorspace = grabber.streamConfiguration().colorSpace.value();

        double gpuTimeAvgMs = 0;

        start_frame_grabber.count_down();
        while (true) {
            printf("Threshold thread!\n");
            auto request = camera_queue.pop();

            if (!request) {
                break;
            }

            auto planes = request->buffers()
                              .at(grabber.streamConfiguration().stream())
                              ->planes();

            for (int i = 0; i < 3; i++) {
                std::cout << "Plane " << (i + 1) << " has fd " << planes[i].fd.get() << " with offset " << planes[i].offset << " and pitch " << static_cast<EGLint>(stride / 2) << std::endl;
            }

            std::array<GlHsvThresholder::DmaBufPlaneData, 3> yuv_data{{
                {planes[0].fd.get(), static_cast<EGLint>(planes[0].offset),
                 static_cast<EGLint>(stride)},
                {planes[1].fd.get(), static_cast<EGLint>(planes[1].offset),
                 static_cast<EGLint>(stride / 2)},
                {planes[2].fd.get(), static_cast<EGLint>(planes[2].offset),
                 static_cast<EGLint>(stride / 2)},
            }};

            auto begintime = steady_clock::now();
            int out = m_thresholder.testFrame(yuv_data,
                                    encodingFromColorspace(colorspace),
                                    rangeFromColorspace(colorspace));
            if (out != 0) {
                gpu_queue.push(out);
            }

            std::chrono::duration<double, std::milli> elapsedMillis =
                steady_clock::now() - begintime;
            if (elapsedMillis > 0.9ms) {
                gpuTimeAvgMs =
                    approxRollingAverage(gpuTimeAvgMs, elapsedMillis.count());
                std::cout << "GLProcess: " << gpuTimeAvgMs << std::endl;
            }

            {
                std::lock_guard<std::mutex> lock{camera_stop_mutex};
                grabber.requeueRequest(request);
            }
        }
    });

    display = std::thread([&]() {
        std::unordered_map<int, unsigned char *> mmaped;

        for (auto fd : fds) {
            auto mmap_ptr = mmap(nullptr, m_width * m_height * 4, PROT_READ,
                                 MAP_SHARED, fd, 0);
            if (mmap_ptr == MAP_FAILED) {
                throw std::runtime_error("failed to mmap pointer");
            }
            mmaped.emplace(fd, static_cast<unsigned char *>(mmap_ptr));
        }

        double copyTimeAvgMs = 0;
        double fpsTimeAvgMs = 0;

        start_frame_grabber.count_down();
        auto lastTime = steady_clock::now();
        while (true) {
            printf("Display thread!\n");
            auto fd = gpu_queue.pop();
            if (fd == -1) {
                break;
            }

            auto mat_pair = MatPair(m_width, m_height);

            uint8_t *processed_out_buf = mat_pair.processed.data;
            uint8_t *color_out_buf = mat_pair.color.data;

            auto begin_time = steady_clock::now();
            auto input_ptr = mmaped.at(fd);
            int bound = m_width * m_height;

            for (int i = 0; i < bound; i++) {
                std::memcpy(color_out_buf + i * 3, input_ptr + i * 4, 3);
                processed_out_buf[i] = input_ptr[i * 4 + 3];
            }

            m_thresholder.returnBuffer(fd);
            outgoing.set(std::move(mat_pair));

            std::chrono::duration<double, std::milli> elapsedMillis =
                steady_clock::now() - begin_time;
            copyTimeAvgMs =
                approxRollingAverage(copyTimeAvgMs, elapsedMillis.count());
            std::cout << "Copy: " << copyTimeAvgMs << std::endl;

            auto now = steady_clock::now();
            std::chrono::duration<double, std::milli> elapsed =
                (now - lastTime);
            fpsTimeAvgMs = approxRollingAverage(fpsTimeAvgMs, elapsed.count());
            printf("Delta %.2f FPS: %.2f\n", fpsTimeAvgMs,
                   1000.0 / fpsTimeAvgMs);
            lastTime = now;
        }

        for (const auto &[fd, pointer] : mmaped) {
            munmap(pointer, m_width * m_height * 4);
        }
    });

    start_frame_grabber.wait();

    {
        std::lock_guard<std::mutex> lock{camera_stop_mutex};
        grabber.startAndQueue();
    }
}

void CameraRunner::stop() {
    printf("stopping all\n");
    // stop the camera
    {
        std::lock_guard<std::mutex> lock{camera_stop_mutex};
        grabber.stop();
    }

    // push sentinel value to stop threshold thread
    camera_queue.push(nullptr);
    threshold.join();

    // push sentinel value to stop display thread
    gpu_queue.push(-1);
    display.join();

    printf("stopped all\n");
}
