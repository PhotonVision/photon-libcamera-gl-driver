#!/bin/bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

sysroot_dir="${SYSROOT_DIR:-}"
if [ -z "${sysroot_dir}" ]; then
  echo "SYSROOT_DIR is required (path to the target rootfs or sysroot)." 1>&2
  exit 1
fi
sysroot_dir="$(realpath "${sysroot_dir}")"

docker_base_image="${DOCKER_BASE_IMAGE:-debian:bookworm}"
docker_image="${DOCKER_IMAGE:-photon-libcamera-gl-driver-arm64-jni:bookworm}"
dockerfile="${DOCKERFILE:-${repo_root}/tools/docker/arm64-jni.Dockerfile}"
docker_platform="${DOCKER_PLATFORM:-linux/arm64}"
build_docker_image="${BUILD_DOCKER_IMAGE:-1}"
maven_local_repo="${MAVEN_LOCAL_REPO:-${repo_root}/.m2-arm64}"
cmake_extra_args="${CMAKE_EXTRA_ARGS:-}"
gradle_extra_args="${GRADLE_EXTRA_ARGS:-}"
clean_build="${CLEAN_BUILD:-0}"
sysroot_mode="${SYSROOT_MODE:-libcamera}"
use_docker="${USE_DOCKER:-1}"

if [ "${clean_build}" = "1" ]; then
  rm -rf "${repo_root}/cmake_build" || true
fi

mkdir -p "${maven_local_repo}"

if [ "${use_docker}" != "1" ]; then
  export DEBIAN_FRONTEND=noninteractive
  export PKG_CONFIG_SYSROOT_DIR="${sysroot_dir}"
  export PKG_CONFIG_PATH="${sysroot_dir}/usr/lib/aarch64-linux-gnu/pkgconfig:${sysroot_dir}/usr/lib/pkgconfig:${sysroot_dir}/usr/share/pkgconfig:${sysroot_dir}/usr/local/lib/aarch64-linux-gnu/pkgconfig:${sysroot_dir}/usr/local/lib/pkgconfig:/usr/lib/aarch64-linux-gnu/pkgconfig:/usr/lib/pkgconfig:/usr/share/pkgconfig"
  if [ -z "${JAVA_HOME:-}" ]; then
    arch="$(dpkg --print-architecture 2>/dev/null || true)"
    if [ "${arch}" = "amd64" ]; then
      export JAVA_HOME="/usr/lib/jvm/java-17-openjdk-amd64"
    else
      export JAVA_HOME="/usr/lib/jvm/java-17-openjdk-arm64"
    fi
  else
    export JAVA_HOME="${JAVA_HOME}"
  fi
  git config --global --add safe.directory "${repo_root}"

  build_dir="${repo_root}/cmake_build"
  cmake_args=(
    -B "${build_dir}"
    -S "${repo_root}"
    -DOpenGL_GL_PREFERENCE=GLVND
    -DBUILD_CAMERA_MEME=OFF
  )
  if [ "${sysroot_mode}" = "full" ]; then
    cmake_args+=(
      -DCMAKE_SYSROOT="${sysroot_dir}"
      -DCMAKE_FIND_ROOT_PATH="${sysroot_dir}"
      -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY
      -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY
      -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY
      -DCMAKE_PREFIX_PATH="${sysroot_dir}/usr;${sysroot_dir}/usr/local"
    )
  else
    cmake_args+=(
      -DCMAKE_LIBRARY_PATH="${sysroot_dir}/usr/lib/aarch64-linux-gnu:${sysroot_dir}/usr/local/lib/aarch64-linux-gnu"
      -DCMAKE_INCLUDE_PATH="${sysroot_dir}/usr/include:${sysroot_dir}/usr/local/include"
    )
  fi
  cmake "${cmake_args[@]}" ${cmake_extra_args}
  cmake --build "${build_dir}" -j"$(nproc)"

  cd "${repo_root}"
  ./gradlew --no-daemon \
    -Dmaven.repo.local="${maven_local_repo}" \
    build publishToMavenLocal \
    -PArchOverride=linuxarm64 \
    ${gradle_extra_args}
  exit 0
fi

if [ "${build_docker_image}" = "1" ]; then
  docker build \
    --platform="${docker_platform}" \
    --build-arg "BASE_IMAGE=${docker_base_image}" \
    -f "${dockerfile}" \
    -t "${docker_image}" \
    "${repo_root}"
fi

docker run --rm --platform="${docker_platform}" \
  --user "$(id -u):$(id -g)" \
  -v "${repo_root}:/work" \
  -v "${sysroot_dir}:/sysroot:ro" \
  -v "${maven_local_repo}:/work/.m2-arm64" \
  -e HOME=/tmp \
  -e "SYSROOT_MODE=${sysroot_mode}" \
  -e "CMAKE_EXTRA_ARGS=${cmake_extra_args}" \
  -e "GRADLE_EXTRA_ARGS=${gradle_extra_args}" \
  "${docker_image}"
