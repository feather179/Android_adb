cmake_minimum_required(VERSION 3.21)

project(base VERSION 1.0.0 LANGUAGES CXX)

set(${PROJECT_NAME}_SRCS
    abi_compatibility.cpp
    chrono_utils.cpp
    cmsg.cpp
    file.cpp
    hex.cpp
    logging.cpp
    mapped_file.cpp
    parsebool.cpp
    parsenetaddress.cpp
    posix_strerror_r.cpp
    process.cpp
    # properties.cpp
    result.cpp
    stringprintf.cpp
    strings.cpp
    threads.cpp
    test_utils.cpp
)

add_library(${PROJECT_NAME} STATIC ${${PROJECT_NAME}_SRCS})

target_include_directories(${PROJECT_NAME} PUBLIC include)

target_compile_options(${PROJECT_NAME} PRIVATE -Wno-c99-designator)

target_link_libraries(${PROJECT_NAME} PUBLIC log-header-only fmt-header-only)

