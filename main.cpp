#include "camera_runner.h"
#include "libcamera_jni.hpp"

#include <chrono>
#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

enum class ProcessType_: int32_t {
    None = 0,
    Hsv,
    Gray,
    Adaptive,
};

void test_res(int width, int height) {
    Java_org_photonvision_raspi_LibCameraJNI_createCamera(nullptr, nullptr,
                                                          width, height, 30);
    // Java_org_photonvision_raspi_LibCameraJNI_setGpuProcessType(nullptr, nullptr, 1);
    Java_org_photonvision_raspi_LibCameraJNI_setGpuProcessType(nullptr, nullptr, (jint)ProcessType_::Adaptive);
    Java_org_photonvision_raspi_LibCameraJNI_setFramesToCopy(nullptr, nullptr, true, true);
    Java_org_photonvision_raspi_LibCameraJNI_startCamera(nullptr, nullptr);

    Java_org_photonvision_raspi_LibCameraJNI_setExposure(nullptr, nullptr, 20 * 800);
    Java_org_photonvision_raspi_LibCameraJNI_setBrightness(nullptr, nullptr, 0.0);
    Java_org_photonvision_raspi_LibCameraJNI_setAnalogGain(nullptr, nullptr, 10);

    auto start = std::chrono::steady_clock::now();

    while (std::chrono::steady_clock::now() - start < std::chrono::seconds(3))  {
        bool ready = Java_org_photonvision_raspi_LibCameraJNI_awaitNewFrame(nullptr, nullptr);
        if (ready) {
            static int i = 0;

            cv::Mat color_mat = *(cv::Mat*)Java_org_photonvision_raspi_LibCameraJNI_takeColorFrame(nullptr, nullptr);
            cv::Mat threshold_mat = *(cv::Mat*)Java_org_photonvision_raspi_LibCameraJNI_takeProcessedFrame(nullptr, nullptr);

            i++;
            static char arr[50];
            snprintf(arr,sizeof(arr),"color_%i.png", i);
            cv::imwrite(arr, color_mat);
            snprintf(arr,sizeof(arr),"thresh_%i.png", i);
            cv::imwrite(arr, threshold_mat);
        }
    }

    Java_org_photonvision_raspi_LibCameraJNI_stopCamera(nullptr, nullptr);
    Java_org_photonvision_raspi_LibCameraJNI_destroyCamera(nullptr, nullptr);

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
}

int main() {
    // test_res(1920, 1080);
    // test_res(320, 240);
    // test_res(640, 480);
    test_res(960, 720);
    // // test_res(2592, 1944);
    // test_res(2592/2, 1944/2);
    // test_res(1920, 1080);
    // test_res(320, 240);
    // test_res(640, 480);
    // test_res(960, 720);
    // // test_res(2592, 1944);
    // test_res(2592/2, 1944/2);

    std::cout << "Done" << std::endl;

    return 0;
}