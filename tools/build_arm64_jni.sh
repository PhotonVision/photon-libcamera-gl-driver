#!/bin/bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

sysroot_dir="${SYSROOT_DIR:-}"
if [ -z "${sysroot_dir}" ]; then
  echo "SYSROOT_DIR is required (path to the target rootfs or sysroot)." 1>&2
  exit 1
fi
sysroot_dir="$(realpath "${sysroot_dir}")"

docker_image="${DOCKER_IMAGE:-debian:bookworm}"
docker_platform="${DOCKER_PLATFORM:-linux/arm64}"
maven_local_repo="${MAVEN_LOCAL_REPO:-${repo_root}/.m2-arm64}"
cmake_extra_args="${CMAKE_EXTRA_ARGS:-}"
gradle_extra_args="${GRADLE_EXTRA_ARGS:-}"
clean_build="${CLEAN_BUILD:-0}"
sysroot_mode="${SYSROOT_MODE:-libcamera}"

if [ "${clean_build}" = "1" ]; then
  rm -rf "${repo_root}/cmake_build" || true
fi

mkdir -p "${maven_local_repo}"

docker run --rm --platform="${docker_platform}" \
  -v "${repo_root}:/work" \
  -v "${sysroot_dir}:/sysroot:ro" \
  -v "${maven_local_repo}:/work/.m2-arm64" \
  "${docker_image}" bash -lc "
    set -euo pipefail
    export DEBIAN_FRONTEND=noninteractive
    apt-get update >/dev/null
    apt-get install -y --no-install-recommends \
      build-essential \
      ca-certificates \
      cmake \
      git \
      libdrm-dev \
      libegl1-mesa-dev \
      libgbm-dev \
      libgl1-mesa-dev \
      ninja-build \
      openjdk-17-jdk \
      pkg-config >/dev/null

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
    if [ \"${sysroot_mode}\" = \"full\" ]; then
      cmake_args+=(
        -DCMAKE_SYSROOT=/sysroot
        -DCMAKE_FIND_ROOT_PATH=/sysroot
        -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY
        -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY
        -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY
        -DCMAKE_PREFIX_PATH=/sysroot/usr\\;/sysroot/usr/local
      )
    else
      cmake_args+=(
        -DCMAKE_LIBRARY_PATH=/sysroot/usr/lib/aarch64-linux-gnu:/sysroot/usr/local/lib/aarch64-linux-gnu
        -DCMAKE_INCLUDE_PATH=/sysroot/usr/include:/sysroot/usr/local/include
      )
    fi
    cmake \${cmake_args[@]} ${cmake_extra_args}
    cmake --build /work/cmake_build -j\$(nproc)

    cd /work
    ./gradlew --no-daemon \
      -Dmaven.repo.local=/work/.m2-arm64 \
      build publishToMavenLocal \
      -PArchOverride=linuxarm64 \
      ${gradle_extra_args}
  "
