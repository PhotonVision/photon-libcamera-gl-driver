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

#include <chrono>
#include <iostream>
#include <thread>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "camera_manager.h"
#include "camera_runner.h"
#include "glerror.h"
#include "headless_opengl.h"
#include "libcamera_jni.hpp"

#define GLERROR() glerror(__LINE__)
#define EGLERROR() eglerror(__LINE__)

void test_res(int width, int height) {

    std::vector<std::shared_ptr<libcamera::Camera>> cameras = GetAllCameraIDs();

    // Yeet all USB cameras (I hope)
    auto rem = std::remove_if(cameras.begin(), cameras.end(), [](auto &cam) {
        return cam->id().find("/usb") != std::string::npos;
    });
    cameras.erase(rem, cameras.end());

    for (const auto &cam : cameras) {
        std::printf("Camera at: %s\n", cam->id().c_str());
    }

    std::vector<CameraRunner *> runners;

    int rotation = 0;

    for (auto &c : cameras) {
        auto r = new CameraRunner(width, height, rotation, c);
        runners.push_back(r);
        r->start();
        r->setCopyOptions(true, true);
        r->requestShaderIdx((int)ProcessType::Gray);

        r->cameraGrabber().cameraSettings().exposureTimeUs = 100000;
        r->cameraGrabber().cameraSettings().analogGain = 4;
        r->cameraGrabber().cameraSettings().brightness = 0.0;

        std::printf("Started %s!\n", c->id().c_str());
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    for (int i = 0; i < 50; i++) {
        int j = 0;
        for (auto r : runners) {
            auto pair = r->outgoing.take();
            auto color_mat = cv::Mat(std::move(pair.color));
            auto processed_mat = cv::Mat(std::move(pair.processed));

            auto now =
                Java_org_photonvision_raspi_LibCameraJNI_getLibcameraTimestamp(
                    nullptr, NULL);
            auto then =
                Java_org_photonvision_raspi_LibCameraJNI_getFrameCaptureTime(
                    nullptr, NULL, (long int)&pair);

            std::printf("now %li then %li dt %i\n", now, then, now - then);

            if (i % 30 == 0) {
                std::printf("saving cam %i idx %i\n", j, i);
                cv::Mat bgr;
                cv::cvtColor(processed_mat, bgr, cv::COLOR_GRAY2BGR);
                static char arr[50];
                std::snprintf(arr, sizeof(arr), "color_cam%i_%i_%ix%i.png", j,
                              i, width, height);
                cv::imwrite(arr, color_mat);
                std::snprintf(arr, sizeof(arr), "thresh_cam%i_%i_%ix%i.png", j,
                              i, width, height);
                cv::imwrite(arr, bgr);
            }

            color_mat.release();
            processed_mat.release();

            j++;
        }
    }

    std::printf("Destroying all!\n");
    for (auto &r : runners) {
        r->stop();
        delete r;
    }
}

int main() {

    for (int i = 0; i < 1; i++) {
        test_res(1280, 800);
        // test_res(1280/2, 800/2);
        // test_res(640, 480);
    }

    std::cout << "Done" << std::endl;

    return 0;
}
