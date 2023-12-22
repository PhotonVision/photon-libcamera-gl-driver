#include "camera_runner.h"
#include "libcamera_jni.hpp"

#include <chrono>
#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

#include "camera_manager.h"

#include <chrono>
#include <iostream>
#include <thread>

#include "headless_opengl.h"
#include "glerror.h"

#define GLERROR() glerror(__LINE__)
#define EGLERROR() eglerror(__LINE__)

enum class ProcessType_: int32_t {
    None = 0,
    Hsv,
    Gray,
    Adaptive,
};

void test_res(int width, int height) {

    std::vector<std::shared_ptr<libcamera::Camera>> cameras = GetAllCameraIDs();

    // Yeet all USB cameras (I hope)
    auto rem = std::remove_if(cameras.begin(), cameras.end(), [](auto &cam) {
        return cam->id().find("/usb") != std::string::npos;
    });
    cameras.erase(rem, cameras.end());

    for (const auto& cam : cameras) {
        printf("Camera at: %s\n", cam->id().c_str());
    }

    std::vector<CameraRunner*> runners;

    int rotation = 0;

    for (auto& c : cameras) {
        auto r = new CameraRunner(width, height, rotation, c);
        runners.push_back(r);
        r->start();
        r->setCopyOptions(true, true);
        r->requestShaderIdx((int)ProcessType::Gray);

        r->cameraGrabber().cameraSettings().exposureTimeUs = 8000;
        r->cameraGrabber().cameraSettings().analogGain = 3;
        r->cameraGrabber().cameraSettings().brightness = 0.3;

        printf("Started %s!\n", c->id().c_str());
    }

    // std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    for (int i = 0; i < 20; i++) {
        int j = 0;
        for (auto r : runners) {
            auto pair = r->outgoing.take();
            auto color_mat = cv::Mat(std::move(pair.color));
            auto processed_mat = cv::Mat(std::move(pair.processed));

            if (i % 30 == 0) {
                printf("saving cam %i idx %i\n", j, i);
                static char arr[50];
                snprintf(arr,sizeof(arr),"color_cam%i_%i_%ix%i.png", j, i, width, height);
                cv::imwrite(arr, color_mat);
                snprintf(arr,sizeof(arr),"thresh_cam%i_%i_%ix%i.png", j, i, width, height);
                cv::imwrite(arr, processed_mat);
            }

            color_mat.release();
            processed_mat.release();

            j++;
        }
    }

    printf("Destroying all!\n");
    for (auto& r : runners) {
        r->stop();
        delete r;
    }
}

int main() {
    
    for (int i = 0; i < 100; i++) {
        test_res(1280, 800);
        test_res(1280/2, 800/2);
        test_res(640, 480);
    }

    std::cout << "Done" << std::endl;

    return 0;
}