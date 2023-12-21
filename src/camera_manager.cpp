#include "camera_manager.h"

std::vector<std::shared_ptr<libcamera::Camera>> GetAllCameraIDs() {
    static libcamera::CameraManager *camera_manager;
    if (!camera_manager) {
        camera_manager = new libcamera::CameraManager();
        camera_manager->start();
    }
    std::vector<std::shared_ptr<libcamera::Camera>> cams =
        camera_manager->cameras();
    return cams;
}
