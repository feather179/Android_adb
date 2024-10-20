cmake_minimum_required(VERSION 3.21)

project(usb LANGUAGES C)

set(${PROJECT_NAME}_SRCS
    libusb/core.c
    libusb/descriptor.c
    libusb/hotplug.c
    libusb/io.c
    libusb/sync.c
    libusb/strerror.c
    libusb/os/events_posix.c
    libusb/os/linux_usbfs.c
    libusb/os/threads_posix.c
    libusb/os/linux_netlink.c
)

add_library(${PROJECT_NAME} STATIC ${${PROJECT_NAME}_SRCS})

target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE
    android
    libusb
    libusb/os
)

# target_compile_options(${PROJECT_NAME} PRIVATE -Wno-reorder-init-list)

# target_link_libraries(${PROJECT_NAME} PUBLIC
#     base
#     gtest
#     log-header-only
# )

