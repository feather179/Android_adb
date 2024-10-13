cmake_minimum_required(VERSION 3.21)

project(cutils VERSION 1.0.0 LANGUAGES CXX)

set(${PROJECT_NAME}_SRCS
    config_utils.cpp
    iosched_policy.cpp
    load_file.cpp
    native_handle.cpp
    properties.cpp
    record_stream.cpp
    strlcpy.c

    sockets.cpp
    socket_inaddr_any_server_unix.cpp
    socket_local_client_unix.cpp
    socket_local_server_unix.cpp
    socket_network_client_unix.cpp
    sockets_unix.cpp
)

add_library(${PROJECT_NAME} STATIC ${${PROJECT_NAME}_SRCS})

target_include_directories(${PROJECT_NAME} PUBLIC include)

target_link_libraries(${PROJECT_NAME} PUBLIC base)

