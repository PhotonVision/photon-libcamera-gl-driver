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

/* DO NOT EDIT THIS std::FILE - it is machine generated */
#include <jni.h>

/* Header for class org_photonvision_raspi_LibCameraJNI */

#ifndef PHOTON_LIBCAMERA_GL_DRIVER_INCLUDE_LIBCAMERA_JNI_HPP_
#define PHOTON_LIBCAMERA_GL_DRIVER_INCLUDE_LIBCAMERA_JNI_HPP_
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     org_photonvision_raspi_LibCameraJNI
 * Method:    getSensorModelRaw
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jint JNICALL
Java_org_photonvision_raspi_LibCameraJNI_getSensorModelRaw(JNIEnv *, jclass,
                                                           jstring);

/*
 * Class:     org_photonvision_raspi_LibCameraJNI
 * Method:    isVCSMSupported
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL
Java_org_photonvision_raspi_LibCameraJNI_isLibraryWorking(JNIEnv *, jclass);

/*
 * Class:     test
 * Method:    getCameraNames
 * Signature: ()[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL
Java_org_photonvision_raspi_LibCameraJNI_getCameraNames(JNIEnv *, jclass);

/*
 * Class:     org_photonvision_raspi_LibCameraJNI
 * Method:    createCamera
 * Signature: (III)Z
 */
JNIEXPORT jlong JNICALL Java_org_photonvision_raspi_LibCameraJNI_createCamera(
    JNIEnv *, jclass, jstring, jint, jint, jint);

JNIEXPORT jboolean JNICALL
Java_org_photonvision_raspi_LibCameraJNI_startCamera(JNIEnv *, jclass, jlong);

JNIEXPORT jboolean JNICALL
Java_org_photonvision_raspi_LibCameraJNI_stopCamera(JNIEnv *, jclass, jlong);

/*
 * Class:     org_photonvision_raspi_LibCameraJNI
 * Method:    destroyCamera
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL
Java_org_photonvision_raspi_LibCameraJNI_destroyCamera(JNIEnv *, jclass, jlong);

/*
 * Class:     org_photonvision_raspi_LibCameraJNI
 * Method:    setThresholds
 * Signature: (DDDDDD)V
 */
JNIEXPORT jboolean JNICALL
Java_org_photonvision_raspi_LibCameraJNI_setThresholds(JNIEnv *, jclass, jlong,
                                                       jdouble, jdouble,
                                                       jdouble, jdouble,
                                                       jdouble, jdouble,
                                                       jboolean);

/*
 * Class:     org_photonvision_raspi_LibCameraJNI
 * Method:    setExposure
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL Java_org_photonvision_raspi_LibCameraJNI_setExposure(
    JNIEnv *, jclass, jlong, jint);

JNIEXPORT jboolean JNICALL
Java_org_photonvision_raspi_LibCameraJNI_setAutoExposure(
    JNIEnv *env, jclass, jlong, jboolean doAutoExposure);

/*
 * Class:     org_photonvision_raspi_LibCameraJNI
 * Method:    setSaturation
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL
Java_org_photonvision_raspi_LibCameraJNI_setSaturation(JNIEnv *, jclass, jlong,
                                                       jint);

/*
 * Class:     org_photonvision_raspi_LibCameraJNI
 * Method:    setBrightness
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL
Java_org_photonvision_raspi_LibCameraJNI_setBrightness(JNIEnv *, jclass, jlong,
                                                       jdouble);

/*
 * Class:     org_photonvision_raspi_LibCameraJNI
 * Method:    setGain
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL
Java_org_photonvision_raspi_LibCameraJNI_setAnalogGain(JNIEnv *, jclass, jlong,
                                                       jdouble);

JNIEXPORT jboolean JNICALL Java_org_photonvision_raspi_LibCameraJNI_setAwbGain(
    JNIEnv *, jclass, jlong, jdouble red, jdouble blue);

JNIEXPORT jlong JNICALL
Java_org_photonvision_raspi_LibCameraJNI_getLibcameraTimestamp(JNIEnv *,
                                                               jclass);

JNIEXPORT jlong JNICALL
Java_org_photonvision_raspi_LibCameraJNI_getFrameCaptureTime(JNIEnv *, jclass,
                                                             jlong);

/*
 * Class:     org_photonvision_raspi_LibCameraJNI
 * Method:    grabFrame
 * Signature: (Z)J
 */
JNIEXPORT jlong JNICALL
Java_org_photonvision_raspi_LibCameraJNI_awaitNewFrame(JNIEnv *, jclass, jlong);

JNIEXPORT jlong JNICALL Java_org_photonvision_raspi_LibCameraJNI_takeColorFrame(
    JNIEnv *, jclass, jlong);

JNIEXPORT jlong JNICALL
Java_org_photonvision_raspi_LibCameraJNI_takeProcessedFrame(JNIEnv *, jclass,
                                                            jlong);

JNIEXPORT jboolean JNICALL
Java_org_photonvision_raspi_LibCameraJNI_setFramesToCopy(JNIEnv *, jclass,
                                                         jlong, jboolean copyIn,
                                                         jboolean copyOut);

JNIEXPORT jint JNICALL
Java_org_photonvision_raspi_LibCameraJNI_getGpuProcessType(JNIEnv *, jclass,
                                                           jlong);

JNIEXPORT jboolean JNICALL
Java_org_photonvision_raspi_LibCameraJNI_setGpuProcessType(JNIEnv *, jclass,
                                                           jlong, jint);

JNIEXPORT jboolean JNICALL Java_org_photonvision_raspi_LibCameraJNI_releasePair(
    JNIEnv *env, jclass, jlong pair_);

#ifdef __cplusplus
} // extern "C"
#endif
#endif // PHOTON_LIBCAMERA_GL_DRIVER_INCLUDE_LIBCAMERA_JNI_HPP_
