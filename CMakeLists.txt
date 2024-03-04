cmake_minimum_required(VERSION 3.18)
project(libcamera_meme)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED COMPONENTS OpenGL EGL)
pkg_check_modules(LIBDRM REQUIRED libdrm)
pkg_check_modules(LIBCAMERA REQUIRED libcamera)
pkg_check_modules(LIBGBM REQUIRED gbm)

set(OPENCV_YEAR "frc2024")
set(OPENCV_VERSION "4.8.0-2")
# also valid: windowsx86-64

# type can be "", "debug", "static", or "staticdebug"
set(OPENCV_TYPE "")
set(OPENCV_ARCH "linuxarm64")

# Download opencv, and save the path
include(FetchContent)
FetchContent_Declare(
    opencv_lib
    URL https://frcmaven.wpi.edu/artifactory/release/edu/wpi/first/thirdparty/${OPENCV_YEAR}/opencv/opencv-cpp/${OPENCV_VERSION}/opencv-cpp-${OPENCV_VERSION}-${OPENCV_ARCH}${OPENCV_TYPE}.zip
)
FetchContent_MakeAvailable(opencv_lib)

# download OpenCV headers
FetchContent_Declare(
    opencv_header
    URL https://frcmaven.wpi.edu/artifactory/release/edu/wpi/first/thirdparty/${OPENCV_YEAR}/opencv/opencv-cpp/${OPENCV_VERSION}/opencv-cpp-${OPENCV_VERSION}-headers.zip
)
FetchContent_MakeAvailable(opencv_header)

file(GLOB_RECURSE OPENCV_LIB_PATH
  "${opencv_lib_SOURCE_DIR}/**/*.lib"
  "${opencv_lib_SOURCE_DIR}/**/*.so*"
)
SET(OPENCV_INCLUDE_PATH ${opencv_header_SOURCE_DIR})
message("Depending on opencv ${OPENCV_LIB_PATH} / ${OPENCV_INCLUDE_PATH}")

find_package(JNI)
if(JNI_FOUND)
    # Fixes odd AWT dependency
    set(JNI_INCLUDE_DIRS ${JAVA_INCLUDE_PATH} ${JAVA_INCLUDE_PATH2})
    message(STATUS "JNI_INCLUDE_DIRS=${JNI_INCLUDE_DIRS}")
endif()

set(FAST_FLAGS "-Ofast -ftree-vectorize -fPIC")
add_definitions(${FAST_FLAGS})

add_library(
    photonlibcamera
    SHARED
    src/camera_grabber.cpp
    src/dma_buf_alloc.cpp
    src/gl_hsv_thresholder.cpp
    src/libcamera_opengl_utility.cpp
    src/camera_manager.cpp
    src/camera_runner.cpp
    src/camera_model.cpp
    src/headless_opengl.cpp
    src/libcamera_jni.cpp
)
target_compile_definitions(photonlibcamera PUBLIC EGL_NO_X11=1)
target_include_directories(
    photonlibcamera
    PUBLIC
        include
        ${OPENGL_INCLUDE_DIRS}
        ${LIBDRM_INCLUDE_DIRS}
        ${LIBCAMERA_INCLUDE_DIRS}
        ${LIBGBM_INCLUDE_DIRS}
        ${OPENCV_INCLUDE_PATH}
        ${JNI_INCLUDE_DIRS}
)
target_link_libraries(
    photonlibcamera
    PUBLIC
        OpenGL::GL
        OpenGL::EGL
        Threads::Threads
        ${LIBCAMERA_LINK_LIBRARIES}
        ${LIBGBM_LINK_LIBRARIES}
        ${OPENCV_LIB_PATH}
        ${LIBDRM_LINK_LIBRARIES}
)
target_compile_options(photonlibcamera PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_executable(libcamera_meme main.cpp)
target_include_directories(
    libcamera_meme
    PUBLIC
        ${OPENGL_INCLUDE_DIRS}
        ${LIBDRM_INCLUDE_DIRS}
        ${LIBCAMERA_INCLUDE_DIRS}
        ${LIBGBM_INCLUDE_DIRS}
        ${OPENCV_INCLUDE_PATH}
        ${JNI_INCLUDE_DIRS}
)
target_link_libraries(libcamera_meme photonlibcamera)
