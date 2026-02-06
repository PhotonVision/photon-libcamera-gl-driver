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

#include <algorithm>
#include <cstring>
#include <string>
#include <string_view>

static const CameraModel grayScaleCameras[] = {OV9281, OV7251};

CameraModel stringToModel(std::string_view model) {
    if (model == "ov5647")
        return OV5647;
    else if (model == "imx219")
        return IMX219;
    else if (model == "imx708")
        return IMX708;
    else if (model == "imx477")
        return IMX477;
    else if (model == "ov9281")
        return OV9281;
    else if (model == "ov9782")
        return OV9782;
    else if (model == "ov7251")
        return OV7251;
    else if (model == "Disconnected")
        return Disconnected;
    else
        return Unknown;
}

bool isGrayScale(CameraModel model) {
    return std::find(std::begin(grayScaleCameras), std::end(grayScaleCameras),
                     model) != std::end(grayScaleCameras);
}
