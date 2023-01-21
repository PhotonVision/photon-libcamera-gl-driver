/*
 * Copyright (C) 2022 Photon Vision.
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

#include "libcamera_jni.hpp" // Generated
#include <libcamera/property_ids.h>

#include "camera_manager.h"
#include "camera_runner.h"

static CameraRunner *runner = nullptr;

extern "C" {

#include <jni.h>

// We use jlongs like pointers, so they better be large enough
static_assert(sizeof(void *) <= sizeof(jlong));

JNIEXPORT jboolean
Java_org_photonvision_raspi_LibCameraJNI_isLibraryWorking(JNIEnv *env, jclass) {
    // TODO
    return true;
}

JNIEXPORT jboolean JNICALL
Java_org_photonvision_raspi_LibCameraJNI_createCamera(JNIEnv *env, jclass,
                                                      jint width, jint height,
                                                      jint rotation) {

    std::vector<std::shared_ptr<libcamera::Camera>> cameras = GetAllCameraIDs();

    // Yeet all USB cameras (I hope)
    auto rem = std::remove_if(cameras.begin(), cameras.end(), [](auto &cam) {
        return cam->id().find("/usb") != std::string::npos;
    });
    cameras.erase(rem, cameras.end());

    if (cameras.empty()) {
        runner = 0;
        return false;
    }

    // Otherwise, just create the first camera left
    runner = new CameraRunner(width, height, rotation, cameras[0]);
    return true;
}

JNIEXPORT jint Java_org_photonvision_raspi_LibCameraJNI_getSensorModelRaw(
    JNIEnv *env, jclass clazz) {

    bool runner_exists = runner != nullptr;
    if (!runner_exists) {
        Java_org_photonvision_raspi_LibCameraJNI_createCamera(env, clazz, 320,
                                                              240, 30);
    }

    if (!runner) {
        return 0;
    }

    jint model = runner->model();

    if (!runner_exists) {
        Java_org_photonvision_raspi_LibCameraJNI_destroyCamera(env, clazz);
    }

    return model;
}

JNIEXPORT jboolean JNICALL
Java_org_photonvision_raspi_LibCameraJNI_startCamera(JNIEnv *, jclass) {
    if (!runner) {
        return false;
    }

    runner->start();
    return true;
}

JNIEXPORT jboolean JNICALL
Java_org_photonvision_raspi_LibCameraJNI_stopCamera(JNIEnv *, jclass) {
    if (!runner) {
        return false;
    }

    runner->stop();
    return true;
}

JNIEXPORT jboolean JNICALL
Java_org_photonvision_raspi_LibCameraJNI_destroyCamera(JNIEnv *env, jclass) {
    if (!runner) {
        return false;
    }

    delete runner;
    runner = nullptr;
    return true;
}

JNIEXPORT jboolean JNICALL
Java_org_photonvision_raspi_LibCameraJNI_setThresholds(JNIEnv *env, jclass,
                                                       jdouble hl, jdouble sl,
                                                       jdouble vl, jdouble hu,
                                                       jdouble su, jdouble vu,
                                                       jboolean hueInverted) {
    if (!runner) {
        return false;
    }

    // printf("Setting HSV to %f-%f %f-%f %f-%f\n", hl, hu, sl, su, vl, vu);

    // TODO hue inversion
    runner->thresholder().setHsvThresholds(hl, sl, vl, hu, su, vu, hueInverted);
    return true;
}

JNIEXPORT jboolean JNICALL Java_org_photonvision_raspi_LibCameraJNI_setExposure(
    JNIEnv *env, jclass, jint exposure) {
    if (!runner) {
        return false;
    }

    runner->cameraGrabber().cameraSettings().exposureTimeUs = exposure;
    return true;
}

JNIEXPORT jboolean JNICALL
Java_org_photonvision_raspi_LibCameraJNI_setAutoExposure(
    JNIEnv *env, jclass, jboolean doAutoExposure) {
    if (!runner) {
        return false;
    }

    runner->cameraGrabber().cameraSettings().doAutoExposure = doAutoExposure;
    return true;
}

JNIEXPORT jboolean JNICALL
Java_org_photonvision_raspi_LibCameraJNI_setBrightness(JNIEnv *env, jclass,
                                                       jdouble brightness) {
    if (!runner) {
        return false;
    }

    runner->cameraGrabber().cameraSettings().brightness = brightness;
    return true;
}

JNIEXPORT jboolean JNICALL Java_org_photonvision_raspi_LibCameraJNI_setAwbGain(
    JNIEnv *env, jclass, jdouble red, jdouble blue) {
    if (!runner) {
        return false;
    }

    printf("Setting red %f blue %f\n", (float)red, (float)blue);

    runner->cameraGrabber().cameraSettings().awbRedGain = red;
    runner->cameraGrabber().cameraSettings().awbBlueGain = blue;
    return true;
}

JNIEXPORT jboolean JNICALL
Java_org_photonvision_raspi_LibCameraJNI_setAnalogGain(JNIEnv *env, jclass,
                                                       jdouble analog) {
    if (!runner) {
        return false;
    }

    runner->cameraGrabber().cameraSettings().analogGain = analog;
    return true;
}

JNIEXPORT jboolean JNICALL Java_org_photonvision_raspi_LibCameraJNI_setRotation(
    JNIEnv *env, jclass, jint rotationOrdinal) {
    if (!runner) {
        return false;
    }

    // int rotation = (rotationOrdinal + 3) * 90; // Degrees
    // TODO
    return true;
}

JNIEXPORT jboolean JNICALL
Java_org_photonvision_raspi_LibCameraJNI_setFramesToCopy(JNIEnv *, jclass,
                                                         jboolean copyIn,
                                                         jboolean copyOut) {
    if (!runner) {
        return false;
    }

    runner->setCopyOptions(copyIn, copyOut);
    return true;
}

static MatPair pair = {};

JNIEXPORT jlong JNICALL
Java_org_photonvision_raspi_LibCameraJNI_getFrameCaptureTime(JNIEnv *env,
                                                             jclass) {
    return pair.captureTimestamp;
}

JNIEXPORT jlong JNICALL
Java_org_photonvision_raspi_LibCameraJNI_getLibcameraTimestamp(JNIEnv *env,
                                                               jclass) {
    timespec ts;
    clock_gettime(CLOCK_BOOTTIME, &ts);
    uint64_t now_nsec = (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
    return (jlong)now_nsec;
}

JNIEXPORT jboolean JNICALL
Java_org_photonvision_raspi_LibCameraJNI_awaitNewFrame(JNIEnv *env, jclass) {
    if (!runner) {
        // NULL
        return false;
    }

    pair = runner->outgoing.take();
    return true;
}

JNIEXPORT jlong JNICALL
Java_org_photonvision_raspi_LibCameraJNI_takeColorFrame(JNIEnv *env, jclass) {
    if (!runner) {
        // NULL
        return 0;
    }

    return reinterpret_cast<jlong>(new cv::Mat(std::move(pair.color)));
}

JNIEXPORT jlong JNICALL
Java_org_photonvision_raspi_LibCameraJNI_takeProcessedFrame(JNIEnv *env,
                                                            jclass) {
    if (!runner) {
        // NULL
        return 0;
    }

    return reinterpret_cast<jlong>(new cv::Mat(std::move(pair.processed)));
}

JNIEXPORT jboolean JNICALL
Java_org_photonvision_raspi_LibCameraJNI_setGpuProcessType(JNIEnv *env, jclass,
                                                           jint idx) {
    if (!runner) {
        return false;
    }

    runner->requestShaderIdx(idx);

    return true;
}

JNIEXPORT jint JNICALL
Java_org_photonvision_raspi_LibCameraJNI_getGpuProcessType(JNIEnv *, jclass) {
    return pair.frameProcessingType;
}

JNIEXPORT jboolean JNICALL
Java_org_photonvision_raspi_LibCameraJNI_setAutoFocus(JNIEnv *env, jclass,
                                                      jboolean doAutoFocus) {
    if (!runner) {
        return false;
    }

    runner->cameraGrabber().cameraSettings().doAutoFocus = doAutoFocus;
    return true;
}

} // extern "C"
