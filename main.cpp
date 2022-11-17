#include "camera_runner.h"
#include "libcamera_jni.hpp"

#include <chrono>
#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

int main() {
    Java_org_photonvision_raspi_LibCameraJNI_createCamera(nullptr, nullptr,
                                                          320, 240, 30);
    Java_org_photonvision_raspi_LibCameraJNI_setGpuProcessType(nullptr, nullptr, 0);
    Java_org_photonvision_raspi_LibCameraJNI_startCamera(nullptr, nullptr);

    auto start = std::chrono::steady_clock::now();

    while (std::chrono::steady_clock::now() - start < std::chrono::seconds(30))  {
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

    std::cout << "Done" << std::endl;

    return 0;
}