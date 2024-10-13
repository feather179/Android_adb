cmake_minimum_required(VERSION 3.21)

project(ziparchive LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(${PROJECT_NAME}_SRCS
    zip_archive.cc
    zip_archive_stream_entry.cc
    zip_cd_entry_map.cc
    zip_error.cpp
    zip_writer.cc
    incfs_support/signal_handling.cpp
)

add_library(${PROJECT_NAME} STATIC ${${PROJECT_NAME}_SRCS})

target_include_directories(${PROJECT_NAME} PUBLIC include)
target_include_directories(${PROJECT_NAME} PRIVATE . incfs_support/include)

target_compile_options(${PROJECT_NAME} PRIVATE -Wno-reorder-init-list)

target_link_libraries(${PROJECT_NAME} PUBLIC
    base
    gtest
    log-header-only
)

