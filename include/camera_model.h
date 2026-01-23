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

#pragma once

#include <string>
#include <vector>

enum CameraModel {
    Disconnected = 0,
    OV5647, // Picam v1
    IMX219, // Picam v2
    IMX708, // Picam v3
    IMX477, // Picam HQ
    OV9281,
    OV7251,
    OV9782,
    Unknown
};
CameraModel stringToModel(const std::string &model);
bool isGrayScale(CameraModel model);
