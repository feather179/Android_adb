cmake_minimum_required(VERSION 3.21)

file(TO_CMAKE_PATH ${ANDROID_NDK_ROOT} ANDROID_NDK_ROOT_CMAKE_PATH)
file(TO_CMAKE_PATH ${ANDROID_SDK_ROOT} ANDROID_SDK_ROOT_CMAKE_PATH)

set(CMAKE_TOOLCHAIN_FILE ${ANDROID_NDK_ROOT_CMAKE_PATH}/build/cmake/android.toolchain.cmake)
set(ANDROID_ABI arm64-v8a)
set(ANDROID_PLATFORM android-30)
set(ANDROID_JAR ${ANDROID_SDK_ROOT_CMAKE_PATH}/platforms/android-35/android.jar)
set(ANDROID_D8 ${ANDROID_SDK_ROOT_CMAKE_PATH}/build-tools/35.0.0/d8.bat)
set(PROTOC_EXE ${CMAKE_CURRENT_SOURCE_DIR}/protoc.exe)
set(PROTOBUF_JAR ${CMAKE_CURRENT_SOURCE_DIR}/protobuf-java-3.21.12.jar)
set(XXD_EXE ${CMAKE_CURRENT_SOURCE_DIR}/xxd.exe)

project(android-adb VERSION 1.0.0 LANGUAGES C CXX)

add_subdirectory(log)
add_subdirectory(fmtlib)
add_subdirectory(libbase)
add_subdirectory(core)
add_subdirectory(googletest)
add_subdirectory(libziparchive)
add_subdirectory(boringssl)

set(protobuf_BUILD_TESTS OFF)
set(protobuf_BUILD_PROTOC_BINARIES OFF)
add_subdirectory(protobuf)
target_include_directories(libprotobuf PRIVATE ${protobuf_SOURCE_DIR}/android)
target_include_directories(libprotobuf-lite PRIVATE ${protobuf_SOURCE_DIR}/android)

set(LZ4_BUILD_CLI OFF)
add_subdirectory(lz4/build/cmake)

set(ZSTD_BUILD_TESTS OFF)
set(ZSTD_BUILD_PROGRAMS OFF)
add_subdirectory(zstd/build/cmake)

add_subdirectory(brotli)

add_subdirectory(incremental_delivery/incfs)
add_subdirectory(native/libs/binder)
add_subdirectory(base/libs/androidfw)

# add_subdirectory(libusb)
# add_subdirectory(openscreen)
# add_subdirectory(mdnsresponder)

add_subdirectory(adb)
