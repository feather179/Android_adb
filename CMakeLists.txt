cmake_minimum_required(VERSION 3.21)

set(CMAKE_TOOLCHAIN_FILE D:/ProgramFiles/Android/sdk/ndk/27.0.12077973/build/cmake/android.toolchain.cmake)
set(ANDROID_ABI arm64-v8a)
set(ANDROID_PLATFORM android-30)
set(ANDROID_JAR D:/ProgramFiles/Android/sdk/platforms/android-35/android.jar)
set(ANDROID_D8 D:/ProgramFiles/Android/sdk/build-tools/35.0.0/d8.bat)


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
