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
