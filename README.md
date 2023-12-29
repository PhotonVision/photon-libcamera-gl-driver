# GPU-Accelerated, dmabuf-based libcamera JNI

This repository provides GPU accelerated frame capture and preprocessing for Raspberry Pi platforms using libcamera and OpenGL. Currently, shaders exist for GPU-accelerated binary HSV thresholding and greyscaling, though others (such as adaptive threshold, like needed for apriltags) will be supported. This code is exposed via JNI for use in [PhotonVision](https://github.com/PhotonVision/photonvision).

## Building

We use CMake for our builds. The build should output both a shared library, `libphotonlibcamera.so`, and a executable for testing. Start by installing dependencies and cloning the repo:

```
sudo apt-get update
sudo apt-get install -y default-jdk libopencv-dev libegl1-mesa-dev libcamera-dev cmake build-essential libdrm-dev libgbm-dev openjdk-11-jdk

git clone https://github.com/PhotonVision/photon-libcamera-gl-driver.git
```

Build with the following cmake commands:

```
cd photon-libcamera-gl-driver
mkdir cmake_build
cmake -B cmake_build -S .
cmake --build cmake_build
./gradlew build publishtomavenlocal
```

## Running eglinfo

Compile with `g++ -std=c++17 -o eglinfo eglinfo.c headless_opengl.cpp -lEGL -lGLESv2 -lgbm`, and then run with  `./eglinfo`
