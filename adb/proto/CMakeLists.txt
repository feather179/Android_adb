cmake_minimum_required(VERSION 3.21)

set(ADB_PROTO_FILES
    adb_host.proto
    app_processes.proto
    key_type.proto
)

execute_process(
    COMMAND ${PROTOC_EXE} --proto_path ${CMAKE_CURRENT_SOURCE_DIR} --cpp_out=${CMAKE_CURRENT_BINARY_DIR} ${ADB_PROTO_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

