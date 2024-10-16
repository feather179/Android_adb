cmake_minimum_required(VERSION 3.21)

project(adb VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(fastdeploy)
add_subdirectory(proto)

set(LIBADB_SRCS
    adb.cpp
    adb_io.cpp
    adb_listeners.cpp
    # adb_mdns.cpp
    adb_trace.cpp
    adb_unique_fd.cpp
    adb_utils.cpp
    fdevent/fdevent.cpp
    services.cpp
    sockets.cpp
    socket_spec.cpp
    sysdeps/env.cpp
    sysdeps/errno.cpp
    transport.cpp
    transport_fd.cpp
    types.cpp

    # client/openscreen/mdns_service_info.cpp
    # client/openscreen/mdns_service_watcher.cpp
    # client/openscreen/platform/logging.cpp
    # client/openscreen/platform/task_runner.cpp
    # client/openscreen/platform/udp_socket.cpp
    client/auth.cpp
    # client/adb_wifi.cpp
    # client/usb_libusb.cpp
    client/transport_local.cpp
    # client/mdnsresponder_client.cpp
    # client/mdns_utils.cpp
    # client/transport_mdns.cpp
    client/transport_usb.cpp
    # client/pairing/pairing_client.cpp
)

set(LIBADB_POSIX_SRCS
    sysdeps_unix.cpp
    sysdeps/posix/network.cpp
)

set(LIBADB_LINUX_SRCS
    client/usb_linux.cpp
    fdevent/fdevent_epoll.cpp
)

set(FASTDEPLOY_SRCS
    fastdeploy/deploypatchgenerator/apk_archive.cpp
    fastdeploy/deploypatchgenerator/deploy_patch_generator.cpp
    fastdeploy/deploypatchgenerator/patch_utils.cpp
)

set(PROTO_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/proto/adb_host.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/proto/app_processes.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/proto/key_type.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/fastdeploy/proto/ApkEntry.pb.cc
)

set(ADB_CRYPTO_SRCS
    crypto/key.cpp
    crypto/rsa_2048_key.cpp
    crypto/x509_generator.cpp
)

set(ADB_TLS_SRCS
    tls/adb_ca_list.cpp
    tls/tls_connection.cpp
)

set(ADB_PAIRING_CONNECTION_SRCS
    pairing_connection/pairing_connection.cpp
)

set(ADB_PAIRING_AUTH_SRCS
    pairing_auth/aes_128_gcm.cpp
    pairing_auth/pairing_auth.cpp
)


set(${PROJECT_NAME}_SRCS
    client/adb_client.cpp
    client/bugreport.cpp
    client/commandline.cpp
    client/file_sync_client.cpp
    client/main.cpp
    client/console.cpp
    client/adb_install.cpp
    client/line_printer.cpp
    client/fastdeploy.cpp
    client/fastdeploycallbacks.cpp
    client/incremental.cpp
    client/incremental_server.cpp
    client/incremental_utils.cpp
    shell_service_protocol.cpp

    ${LIBADB_SRCS}
    ${LIBADB_POSIX_SRCS}
    ${LIBADB_LINUX_SRCS}
    ${FASTDEPLOY_SRCS}
    ${PROTO_SRCS}
    ${ADB_CRYPTO_SRCS}
    ${ADB_TLS_SRCS}
    # ${ADB_PAIRING_CONNECTION_SRCS}
    # ${ADB_PAIRING_AUTH_SRCS}
)

add_executable(${PROJECT_NAME}
    ${${PROJECT_NAME}_SRCS}
)

add_dependencies(${PROJECT_NAME} deployagent_post_process)

target_compile_definitions(${PROJECT_NAME} PRIVATE ADB_HOST=1)
target_compile_definitions(${PROJECT_NAME} PRIVATE ANDROID_BASE_UNIQUE_FD_DISABLE_IMPLICIT_CONVERSION=1)
target_compile_options(${PROJECT_NAME} PRIVATE -Wno-inconsistent-missing-override)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    crypto/include
    tls/include
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/fastdeploy
    ${CMAKE_CURRENT_BINARY_DIR}/fastdeploy/proto
    ${CMAKE_CURRENT_BINARY_DIR}/proto
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    base
    cutils
    crypto_utils
    diagnose_usb
    androidfw
    libprotobuf
    ziparchive
    crypto
    ssl
    lz4_static
    libzstd_static
    brotlicommon-static
    brotlidec-static
    brotlienc-static
    # usb
    # openscreen
    # mdnssd
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_STRIP} --strip-all $<TARGET_FILE:${PROJECT_NAME}>
)
