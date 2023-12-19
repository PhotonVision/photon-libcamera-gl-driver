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
mkdir build
cd build
cmake ..

make -j4
```

This should spit out the shared library into the build directory.

## Running eglinfo

Compile with `g++ -std=c++17 -o eglinfo eglinfo.c headless_opengl.cpp -lEGL -lGLESv2 -lgbm`, and then run with  `./eglinfo`

## Chroot stuff

Using our docker image and run with --privileged

docker run -it --privileged --mount type=bind,source="$(pwd)",target=/opt/photon_sysroot_v2023.4.2/opt/photon-libcamera-gl-driver photon-libcamera-builder:latest
/opt/photon_sysroot_v2023.4.2/opt/photon-libcamera-gl-driver/start_chroot.sh

Or

docker run -it --privileged --mount type=bind,source="$(pwd)",target=/opt/photon_sysroot_v2023.4.2/opt/photon-libcamera-gl-driver photon-libcamera-builder-entry:latest

# git clone ${{ github.repositoryUrl }} && cd "$(basename "$_" .git)"


rm -rf /var/lib/apt/lists/*
locale-gen --purge POSTIX && echo 'Success!'
