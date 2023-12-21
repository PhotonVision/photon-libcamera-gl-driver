cmake_minimum_required(VERSION 3.18)
project(libcamera_meme)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED COMPONENTS OpenGL EGL)
find_package(OpenCV REQUIRED)
pkg_check_modules(LIBDRM REQUIRED libdrm)
pkg_check_modules(LIBCAMERA REQUIRED libcamera)
pkg_check_modules(LIBGBM REQUIRED gbm)

find_package(JNI)
if (JNI_FOUND)
    # Fixes odd AWT dependency
    set(JNI_INCLUDE_DIRS ${JAVA_INCLUDE_PATH} ${JAVA_INCLUDE_PATH2})
    message(STATUS "JNI_INCLUDE_DIRS=${JNI_INCLUDE_DIRS}")
endif ()

SET(FAST_FLAGS "-Ofast -ftree-vectorize -fPIC")
add_definitions(${FAST_FLAGS})

add_library(photonlibcamera SHARED 
        src/camera_grabber.cpp 
        src/dma_buf_alloc.cpp
        src/gl_hsv_thresholder.cpp
        src/libcamera_opengl_utility.cpp
        src/libcamera_jni.cpp
        src/camera_manager.cpp
        src/camera_runner.cpp
        src/camera_model.cpp
        src/headless_opengl.cpp
)
target_compile_definitions(photonlibcamera PUBLIC EGL_NO_X11=1)
target_include_directories(photonlibcamera PUBLIC include ${OPENGL_INCLUDE_DIRS} ${LIBDRM_INCLUDE_DIRS} ${LIBCAMERA_INCLUDE_DIRS}
        ${LIBGBM_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS} ${JNI_INCLUDE_DIRS})
target_link_libraries(photonlibcamera PUBLIC OpenGL::GL OpenGL::EGL Threads::Threads ${LIBCAMERA_LINK_LIBRARIES}
        ${LIBGBM_LINK_LIBRARIES} ${OpenCV_LIBS} ${LIBDRM_LINK_LIBRARIES})

add_executable(libcamera_meme main.cpp)
target_include_directories(libcamera_meme PUBLIC ${OPENGL_INCLUDE_DIRS} ${LIBDRM_INCLUDE_DIRS} ${LIBCAMERA_INCLUDE_DIRS}
        ${LIBGBM_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS} ${JNI_INCLUDE_DIRS})
target_link_libraries(libcamera_meme photonlibcamera)
