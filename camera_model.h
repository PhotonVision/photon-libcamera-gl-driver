#pragma once

#include <string>

enum CameraModel {
    Disconnected = 0,
    OV5647, // Picam v1
    IMX219, // Picam v2
    IMX477, // Picam HQ
    OV9281,
    OV7251,
    Unknown
};

CameraModel stringToModel(const std::string& model);
