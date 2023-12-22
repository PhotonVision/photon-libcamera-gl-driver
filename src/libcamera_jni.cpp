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
#include "headless_opengl.h"
#include <optional>
#include "camera_model.h"

extern "C" {

#include <jni.h>

// We use jlongs like pointers, so they better be large enough
static_assert(sizeof(void *) <= sizeof(jlong));

JNIEXPORT jboolean
Java_org_photonvision_raspi_LibCameraJNI_isLibraryWorking(JNIEnv *, jclass) {
    // todo
    return true;
}

JNIEXPORT jobjectArray JNICALL Java_org_photonvision_raspi_LibCameraJNI_getCameraNames(JNIEnv *env, jclass) {

    std::vector<std::shared_ptr<libcamera::Camera>> cameras = GetAllCameraIDs();

    // Yeet all USB cameras (I hope)
    auto rem = std::remove_if(cameras.begin(), cameras.end(), [](auto &cam) {
        return cam->id().find("/usb") != std::string::npos;
    });
    cameras.erase(rem, cameras.end());

    jobjectArray ret;

    // https://stackoverflow.com/a/21768693
    ret = (jobjectArray)env->NewObjectArray(cameras.size(),
                                            env->FindClass("java/lang/String"),
                                            NULL);
    for (unsigned int i = 0; i < cameras.size(); i++)
        env->SetObjectArrayElement(ret, i,
                                   env->NewStringUTF(cameras[i]->id().c_str()));

    return (ret);
}

JNIEXPORT jlong JNICALL
Java_org_photonvision_raspi_LibCameraJNI_createCamera(JNIEnv *env, jclass, jstring name,
                                                      jint width, jint height,
                                                      jint rotation) {

    std::vector<std::shared_ptr<libcamera::Camera>> cameras = GetAllCameraIDs();

    const char *c_name = env->GetStringUTFChars(name, 0);

    jlong ret = 0;
    
    // Find our camera by name
    for (auto& c : cameras) {
        if (strcmp(c->id().c_str(), c_name) ==0) {
            ret = reinterpret_cast<jlong>(new CameraRunner(width, height, rotation, cameras[0]));
            break;
        }
    }

    env->ReleaseStringUTFChars(name, c_name);

    return ret;
}

JNIEXPORT jint Java_org_photonvision_raspi_LibCameraJNI_getSensorModelRaw(
    JNIEnv *env, jclass, jstring name) {

    std::vector<std::shared_ptr<libcamera::Camera>> cameras = GetAllCameraIDs();

    const char *c_name = env->GetStringUTFChars(name, 0);

    jint model = Unknown;
    for (auto& c : cameras) {
        if (strcmp(c->id().c_str(), c_name) ==0) {
            model = stringToModel(c->id());
        }
    }

    env->ReleaseStringUTFChars(name, c_name);

    return model;
}

JNIEXPORT jboolean JNICALL
Java_org_photonvision_raspi_LibCameraJNI_startCamera(JNIEnv *, jclass, jlong runner_) {
    CameraRunner *runner = reinterpret_cast<CameraRunner*>(runner_);

    runner->start();
    return true;
}

JNIEXPORT jboolean JNICALL
Java_org_photonvision_raspi_LibCameraJNI_stopCamera(JNIEnv *, jclass, jlong runner_) {
    CameraRunner *runner = reinterpret_cast<CameraRunner*>(runner_);
    if (!runner) {
        return false;
    }

    runner->stop();
    return true;
}

JNIEXPORT jboolean JNICALL
Java_org_photonvision_raspi_LibCameraJNI_destroyCamera(JNIEnv *, jclass, jlong runner_) {
    CameraRunner *runner = reinterpret_cast<CameraRunner*>(runner_);
    if (!runner) {
        return false;
    }

    delete runner;
    runner = nullptr;
    return true;
}

JNIEXPORT jboolean JNICALL
Java_org_photonvision_raspi_LibCameraJNI_setThresholds(JNIEnv *, jclass, jlong runner_,
                                                       jdouble hl, jdouble sl,
                                                       jdouble vl, jdouble hu,
                                                       jdouble su, jdouble vu,
                                                       jboolean hueInverted) {
    CameraRunner *runner = reinterpret_cast<CameraRunner*>(runner_);
    if (!runner) {
        return false;
    }

    // printf("Setting HSV to %f-%f %f-%f %f-%f\n", hl, hu, sl, su, vl, vu);

    // TODO hue inversion
    runner->thresholder().setHsvThresholds(hl, sl, vl, hu, su, vu, hueInverted);
    return true;
}

JNIEXPORT jboolean JNICALL Java_org_photonvision_raspi_LibCameraJNI_setExposure(
    JNIEnv *, jclass, jlong runner_, jint exposure) {
    CameraRunner *runner = reinterpret_cast<CameraRunner*>(runner_);
    if (!runner) {
        return false;
    }

    runner->cameraGrabber().cameraSettings().exposureTimeUs = exposure;
    return true;
}

JNIEXPORT jboolean JNICALL
Java_org_photonvision_raspi_LibCameraJNI_setAutoExposure(
    JNIEnv *, jclass, jlong runner_, jboolean doAutoExposure) {
    CameraRunner *runner = reinterpret_cast<CameraRunner*>(runner_);
    if (!runner) {
        return false;
    }

    runner->cameraGrabber().cameraSettings().doAutoExposure = doAutoExposure;
    return true;
}

JNIEXPORT jboolean JNICALL
Java_org_photonvision_raspi_LibCameraJNI_setBrightness(JNIEnv *, jclass, jlong runner_,
                                                       jdouble brightness) {
    CameraRunner *runner = reinterpret_cast<CameraRunner*>(runner_);
    if (!runner) {
        return false;
    }

    runner->cameraGrabber().cameraSettings().brightness = brightness;
    return true;
}

JNIEXPORT jboolean JNICALL Java_org_photonvision_raspi_LibCameraJNI_setAwbGain(
    JNIEnv *, jclass, jlong runner_, jdouble red, jdouble blue) {
    CameraRunner *runner = reinterpret_cast<CameraRunner*>(runner_);
    if (!runner) {
        return false;
    }

    runner->cameraGrabber().cameraSettings().awbRedGain = red;
    runner->cameraGrabber().cameraSettings().awbBlueGain = blue;
    return true;
}

JNIEXPORT jboolean JNICALL
Java_org_photonvision_raspi_LibCameraJNI_setAnalogGain(JNIEnv *, jclass, jlong runner_,
                                                       jdouble analog) {
    CameraRunner *runner = reinterpret_cast<CameraRunner*>(runner_);
    if (!runner) {
        return false;
    }

    runner->cameraGrabber().cameraSettings().analogGain = analog;
    return true;
}

JNIEXPORT jboolean JNICALL
Java_org_photonvision_raspi_LibCameraJNI_setFramesToCopy(JNIEnv *, jclass, jlong runner_,
                                                         jboolean copyIn,
                                                         jboolean copyOut) {
    CameraRunner *runner = reinterpret_cast<CameraRunner*>(runner_);
    if (!runner) {
        return false;
    }

    runner->setCopyOptions(copyIn, copyOut);
    return true;
}


JNIEXPORT jlong JNICALL
Java_org_photonvision_raspi_LibCameraJNI_getLibcameraTimestamp(JNIEnv *,
                                                               jclass) {
    timespec ts;
    clock_gettime(CLOCK_BOOTTIME, &ts);
    uint64_t now_nsec = (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
    return (jlong)now_nsec;
}

JNIEXPORT jlong JNICALL
Java_org_photonvision_raspi_LibCameraJNI_awaitNewFrame(JNIEnv *, jclass, jlong runner_) {
    CameraRunner *runner = reinterpret_cast<CameraRunner*>(runner_);
    if (!runner) {
        return 0;
    }

    MatPair *pair = new MatPair();
    *pair = runner->outgoing.take();
    return reinterpret_cast<jlong>(pair);
}

JNIEXPORT jlong JNICALL
Java_org_photonvision_raspi_LibCameraJNI_takeColorFrame(JNIEnv *, jclass, jlong pair_) {
    MatPair *pair = reinterpret_cast<MatPair*>(pair_);
    if (!pair) {
        return 0;
    }

    return reinterpret_cast<jlong>(new cv::Mat(std::move(pair->color)));
}

JNIEXPORT jlong JNICALL
Java_org_photonvision_raspi_LibCameraJNI_takeProcessedFrame(JNIEnv *,
                                                            jclass, jlong pair_) {
    MatPair *pair = reinterpret_cast<MatPair*>(pair_);
    if (!pair) {
        return 0;
    }

    return reinterpret_cast<jlong>(new cv::Mat(std::move(pair->processed)));
}

JNIEXPORT jlong JNICALL
Java_org_photonvision_raspi_LibCameraJNI_getFrameCaptureTime(JNIEnv *,
                                                             jclass, jlong pair_) {
    MatPair *pair = reinterpret_cast<MatPair*>(pair_);
    if (!pair) {
        // NULL
        return 0;
    }

    return pair->captureTimestamp;
}

JNIEXPORT jboolean JNICALL
Java_org_photonvision_raspi_LibCameraJNI_releasePair(JNIEnv *,
                                                             jclass, jlong pair_) {
    MatPair *pair = reinterpret_cast<MatPair*>(pair_);
    if (!pair) {
        // NULL
        return false;
    }

    delete pair;
    return true;
}

JNIEXPORT jboolean JNICALL
Java_org_photonvision_raspi_LibCameraJNI_setGpuProcessType(JNIEnv *, jclass, jlong runner_,
                                                           jint idx) {
    CameraRunner *runner = reinterpret_cast<CameraRunner*>(runner_);
    if (!runner) {
        return false;
    }

    runner->requestShaderIdx(idx);

    return true;
}

JNIEXPORT jint JNICALL
Java_org_photonvision_raspi_LibCameraJNI_getGpuProcessType(JNIEnv *, jclass, jlong pair_) {
    MatPair *pair = reinterpret_cast<MatPair*>(pair_);
    if (!pair) {
        // NULL
        return 0;
    }
    return pair->frameProcessingType;
}

} // extern "C"
