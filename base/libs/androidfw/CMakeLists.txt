cmake_minimum_required(VERSION 3.21)

project(androidfw LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(${PROJECT_NAME}_SRCS
    ResourceTypes.cpp
    ZipFileRO.cpp
)

add_library(${PROJECT_NAME} STATIC ${${PROJECT_NAME}_SRCS})

target_include_directories(${PROJECT_NAME} PUBLIC include)

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wno-reorder-init-list
    -Wno-deprecated-anon-enum-enum-conversion
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    base
    cutils
    incfs_utils
    ziparchive
    binder
)

