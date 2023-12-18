FROM ubuntu:22.04
RUN apt-get update && apt-get install -y qemu qemu-user-static binfmt-support libopencv-dev libegl1-mesa-dev libcamera-dev cmake build-essential libdrm-dev libgbm-dev default-jdk openjdk-17-jdk && apt-get clean
COPY photon_sysroot /opt/photon_sysroot
