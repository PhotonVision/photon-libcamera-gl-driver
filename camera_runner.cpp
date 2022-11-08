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

#include <opencv2/core.hpp>
#include <sys/mman.h>
#include <unistd.h>

#include <libcamera/property_ids.h>

using namespace std::chrono;
using namespace std::chrono_literals;

static double approxRollingAverage(double avg, double new_sample) {
    if (avg == 0.0) {
        avg = new_sample / 50;
    } else {
        avg -= avg / 50;
        avg += new_sample / 50;
    }

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

    for (int i = 0; i < 5; i++) {
        m_buffered.push(MatPair{width, height});
    }
}

CameraRunner::~CameraRunner() {
    for (auto i : fds) {
        close(i);
    }
}

void CameraRunner::start() {
    unsigned int stride = grabber.streamConfiguration().stride;

    latch start_frame_grabber{2};

    threshold = std::thread([&]() {
        m_thresholder.start(fds);
        auto colorspace = grabber.streamConfiguration().colorSpace.value();

        m_thresholder.setOnComplete([&](int fd) { gpu_queue.push(fd); });

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

            std::array<GlHsvThresholder::DmaBufPlaneData, 3> yuv_data{{
                {planes[0].fd.get(), static_cast<EGLint>(planes[0].offset),
                 static_cast<EGLint>(stride)},
                {planes[1].fd.get(), static_cast<EGLint>(planes[1].offset),
                 static_cast<EGLint>(stride / 2)},
                {planes[2].fd.get(), static_cast<EGLint>(planes[2].offset),
                 static_cast<EGLint>(stride / 2)},
            }};

            auto begintime = steady_clock::now();
            m_thresholder.testFrame(yuv_data,
                                    encodingFromColorspace(colorspace),
                                    rangeFromColorspace(colorspace));

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

            auto mat_pair = m_buffered.try_pop();

            // If we have no more of our own buffers, try stealing the user
            // buffers. Hopefully we don't steal the last buffer that the user
            // was waiting on due to a race, but we really should have enough
            // buffers that this isn't an issue.
            if (!mat_pair) {
                mat_pair = outgoing.try_pop();
            }

            if (!mat_pair) {
                std::cout << "No more buffers left! Creating new buffer, this "
                             "may lead to OOM conditions."
                          << std::endl;
                mat_pair = MatPair{m_width, m_height};
            }

            unsigned char *threshold_out_buf = mat_pair.value().threshold.data;
            unsigned char *color_out_buf = mat_pair.value().color.data;

            auto begin_time = steady_clock::now();
            auto input_ptr = mmaped.at(fd);
            int bound = m_width * m_height;

            for (int i = 0; i < bound; i++) {
                std::memcpy(color_out_buf + i * 3, input_ptr + i * 4, 3);
                threshold_out_buf[i] = input_ptr[i * 4 + 3];
            }

            m_thresholder.returnBuffer(fd);
            outgoing.push(std::move(mat_pair.value()));

            std::chrono::duration<double, std::milli> elapsedMillis =
                steady_clock::now() - begin_time;
            copyTimeAvgMs =
                approxRollingAverage(copyTimeAvgMs, elapsedMillis.count());
            std::cout << "Copy: " << copyTimeAvgMs << std::endl;

            // static char arr[50];
            // snprintf(arr,sizeof(arr),"color_%i.png", i);
            // cv::imwrite(arr, color_mat);
            // snprintf(arr,sizeof(arr),"thresh_%i.png", i);
            // cv::imwrite(arr, threshold_mat);

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
}

void CameraRunner::requeue_mat(MatPair mat) { m_buffered.push(std::move(mat)); }
