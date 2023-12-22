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

#include <libcamera/control_ids.h>
#include <libcamera/property_ids.h>
#include <linux/dma-buf.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

using steady_clock = std::chrono::steady_clock;
using namespace std::literals::chrono_literals;

static double approxRollingAverage(double avg, double new_sample) {
    avg -= avg / 50;
    avg += new_sample / 50;

    return avg;
}

CameraRunner::CameraRunner(int width, int height, int rotation,
                           std::shared_ptr<libcamera::Camera> cam)
    : m_camera(std::move(cam)), m_width(width), m_height(height),
      grabber(m_camera, m_width, m_height, rotation),
      m_thresholder(m_width, m_height), allocer("/dev/dma_heap/linux,cma") {

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

void CameraRunner::requestShaderIdx(int idx) { m_shaderIdx = idx; }

void CameraRunner::setCopyOptions(bool copyIn, bool copyOut) {
    m_copyInput = copyIn;
    m_copyOutput = copyOut;
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
            // std::printf("Threshold thread!\n");
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

            auto type = static_cast<ProcessType>(m_shaderIdx.load());

            int out = m_thresholder.testFrame(
                yuv_data, encodingFromColorspace(colorspace),
                rangeFromColorspace(colorspace), type);

            if (out != 0) {
                /*
                From libcamera docs:

                The timestamp, expressed in nanoseconds, represents a
                monotonically increasing counter since the system boot time, as
                defined by the Linux-specific CLOCK_BOOTTIME clock id.
                */
                uint64_t sensorTimestamp = static_cast<uint64_t>(
                    request->metadata()
                        .get(libcamera::controls::SensorTimestamp)
                        .value_or(0));

                gpu_queue.push({out, type, sensorTimestamp});
            }

            std::chrono::duration<double, std::milli> elapsedMillis =
                steady_clock::now() - begintime;
            if (elapsedMillis > 0.9ms) {
                gpuTimeAvgMs =
                    approxRollingAverage(gpuTimeAvgMs, elapsedMillis.count());
                // std::cout << "GLProcess: " << elapsedMillis.count() <<
                // std::endl;
            }

            {
                std::lock_guard<std::mutex> lock{camera_stop_mutex};
                grabber.requeueRequest(request);
            }
        }
        m_thresholder.release();
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

        // double copyTimeAvgMs = 0;
        double fpsTimeAvgMs = 0;

        start_frame_grabber.count_down();
        auto lastTime = steady_clock::now();
        while (true) {
            // std::printf("Display thread!\n");
            auto data = gpu_queue.pop();
            if (data.fd == -1) {
                break;
            }

            auto mat_pair = MatPair(m_width, m_height);

            // Save the current shader idx
            mat_pair.frameProcessingType = static_cast<int32_t>(data.type);
            mat_pair.captureTimestamp = data.captureTimestamp;

            uint8_t *processed_out_buf = mat_pair.processed.data;
            uint8_t *color_out_buf = mat_pair.color.data;

            // auto begin_time = steady_clock::now();

            auto input_ptr = mmaped.at(data.fd);
            int bound = m_width * m_height;

            {
                struct dma_buf_sync dma_sync {};
                dma_sync.flags = DMA_BUF_SYNC_START | DMA_BUF_SYNC_RW;
                int ret = ::ioctl(data.fd, DMA_BUF_IOCTL_SYNC, &dma_sync);
                if (ret)
                    throw std::runtime_error("failed to start DMA buf sync");
            }

            if (m_copyInput) {
                for (int i = 0; i < bound; i++) {
                    std::memcpy(color_out_buf + i * 3, input_ptr + i * 4, 3);
                }
            }

            if (m_copyOutput) {
                for (int i = 0; i < bound; i++) {
                    processed_out_buf[i] = input_ptr[i * 4 + 3];
                }
            }

            {
                struct dma_buf_sync dma_sync {};
                dma_sync.flags = DMA_BUF_SYNC_END | DMA_BUF_SYNC_RW;
                int ret = ::ioctl(data.fd, DMA_BUF_IOCTL_SYNC, &dma_sync);
                if (ret)
                    throw std::runtime_error("failed to start DMA buf sync");
            }

            m_thresholder.returnBuffer(data.fd);
            outgoing.set(std::move(mat_pair));

            // std::chrono::duration<double, std::milli> elapsedMillis =
            //     steady_clock::now() - begin_time;
            // copyTimeAvgMs =
            //     approxRollingAverage(copyTimeAvgMs, elapsedMillis.count());
            // std::cout << "Copy: " << copyTimeAvgMs << std::endl;

            auto now = steady_clock::now();
            std::chrono::duration<double, std::milli> elapsed =
                (now - lastTime);
            fpsTimeAvgMs = approxRollingAverage(fpsTimeAvgMs, elapsed.count());
            // std::printf("Delta %.2f FPS: %.2f\n", fpsTimeAvgMs, 1000.0 /
            // fpsTimeAvgMs);
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
    std::printf("stopping all\n");
    // stop the camera
    {
        std::lock_guard<std::mutex> lock{camera_stop_mutex};
        grabber.stop();
    }

    // push sentinel value to stop threshold thread
    camera_queue.push(nullptr);
    threshold.join();

    // push sentinel value to stop display thread
    gpu_queue.push({-1, ProcessType::None, 0});
    display.join();

    std::printf("stopped all\n");
}
