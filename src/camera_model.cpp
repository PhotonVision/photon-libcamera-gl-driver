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

#include "camera_model.h"

#include <cstring>
#include <algorithm>
using namespace std;

CameraModel stringToModel(const std::string &model) {
    std::printf("Checking model: %s\n", model.c_str());
    const char *famname = model.c_str();
    if (!strcmp(famname, "ov5647"))
        return OV5647;
    else if (!strcmp(famname, "imx219"))
        return IMX219;
    else if (!strcmp(famname, "imx708"))
        return IMX708;
    else if (!strcmp(famname, "imx477"))
        return IMX477;
    else if (!strcmp(famname, "ov9281"))
        return OV9281;
    else if (!strcmp(famname, "ov7251"))
        return OV7251;
    else if (!strcmp(famname, "Disconnected"))
        return Disconnected;
    else
        return Unknown;
}

bool isGrayScale(CameraModel model)
{
    if(std::find(std::begin(grayScaleCameras), std::end(grayScaleCameras), model)) {
    return true;
} else {
    return false;
}
}
