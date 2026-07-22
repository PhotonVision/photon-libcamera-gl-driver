#!/bin/bash
set -euo pipefail

sysroot_mode="${SYSROOT_MODE:-libcamera}"
cmake_extra_args="${CMAKE_EXTRA_ARGS:-}"
gradle_extra_args="${GRADLE_EXTRA_ARGS:-}"

export PKG_CONFIG_SYSROOT_DIR=/sysroot
export PKG_CONFIG_PATH=/sysroot/usr/lib/aarch64-linux-gnu/pkgconfig:/sysroot/usr/lib/pkgconfig:/sysroot/usr/share/pkgconfig:/sysroot/usr/local/lib/aarch64-linux-gnu/pkgconfig:/sysroot/usr/local/lib/pkgconfig:/usr/lib/aarch64-linux-gnu/pkgconfig:/usr/lib/pkgconfig:/usr/share/pkgconfig
export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-arm64

git config --global --add safe.directory /work

cmake_args=(
  -B /work/cmake_build
  -S /work
  -DOpenGL_GL_PREFERENCE=GLVND
  -DBUILD_CAMERA_MEME=OFF
)

if [ "${sysroot_mode}" = "full" ]; then
  cmake_args+=(
    -DCMAKE_SYSROOT=/sysroot
    -DCMAKE_FIND_ROOT_PATH=/sysroot
    -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY
    -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY
    -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY
    -DCMAKE_PREFIX_PATH=/sysroot/usr\;/sysroot/usr/local
  )
else
  cmake_args+=(
    -DCMAKE_LIBRARY_PATH=/sysroot/usr/lib/aarch64-linux-gnu:/sysroot/usr/local/lib/aarch64-linux-gnu
    -DCMAKE_INCLUDE_PATH=/sysroot/usr/include:/sysroot/usr/local/include
  )
fi

cmake "${cmake_args[@]}" ${cmake_extra_args}
cmake --build /work/cmake_build -j"$(nproc)"

cd /work
./gradlew --no-daemon \
  -Dmaven.repo.local=/work/.m2-arm64 \
  build publishToMavenLocal \
  -PArchOverride=linuxarm64 \
  ${gradle_extra_args}
