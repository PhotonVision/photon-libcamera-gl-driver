#pragma once

#include <libcamera/camera.h>
#include <libcamera/camera_manager.h>
#include <memory>
#include <string>
#include <vector>

std::vector<std::shared_ptr<libcamera::Camera>> GetAllCameraIDs();
