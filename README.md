# GPU-Accelerated, dmabuf-based libcamera JNI

This repository provides GPU accelerated frame capture and preprocessing for Raspberry Pi platforms using libcamera and OpenGL. Currently, shaders exist for GPU-accelerated binary HSV thresholding and greyscaling, though others (such as adaptive threshold, like needed for apriltags) will be supported. This code is exposed via JNI for use in [PhotonVision](https://github.com/PhotonVision/photonvision).

## Building

We use CMake for our builds. The build should output both a shared library, `libphotonlibcamera.so`, and a executable for testing. Start by installing dependencies and cloning the repo:

```
sudo apt-get update
sudo apt-get install -y default-jdk libopencv-dev libegl1-mesa-dev libcamera-dev cmake build-essential libdrm-dev libgbm-dev openjdk-17-jdk
git clone https://github.com/PhotonVision/photon-libcamera-gl-driver.git
```

Note: Some downstream targets/workflows (including OV9782 on Raspberry Pi images) may
require libcamera >= 0.6. By default, this project will build against whatever
`libcamera-dev` you have installed; to enforce >= 0.6 at configure time, pass:

```
cmake -B cmake_build -S . -DREQUIRE_LIBCAMERA_0_6=ON
```

Build with the following cmake commands:

```
cd photon-libcamera-gl-driver
mkdir cmake_build
cmake -B cmake_build -S .
cmake --build cmake_build
./gradlew build publishToMavenLocal
```

## Arm64 build with sysroot (Docker)

If you are building on an x86_64 host and need an arm64 JNI that links against
libcamera (often >= 0.6) from a target rootfs, run:

```
SYSROOT_DIR=/path/to/target/rootfs \
  MAVEN_LOCAL_REPO=/path/to/m2 \
  tools/build_arm64_jni.sh
```

## Running eglinfo

Compile with `g++ -std=c++17 -o eglinfo eglinfo.c headless_opengl.cpp -lEGL -lGLESv2 -lgbm`, and then run with  `./eglinfo`
